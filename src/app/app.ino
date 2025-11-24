/*
 * ESP32 + E-Paper Display + LVGL + MQTT Project
 * 
 * This project demonstrates:
 * - E-ink display with LVGL integration
 * - WiFi connectivity
 * - MQTT subscription for real-time data
 * - Multi-screen architecture with lifecycle management
 * - SquareLine Studio compatible screen pattern
 * - Memory-efficient LVGL integration
 * - Manual refresh control for low power operation
 * 
 * HARDWARE:
 * - ESP32 Dev Module
 * - WeAct Studio 1.54" E-Paper Module (GDEH0154D67, 200x200, B/W)
 * - See docs/pinout.md for wiring details
 * 
 * LIBRARIES:
 * - GxEPD2: E-ink display driver
 * - LVGL: Graphics library
 * - Adafruit GFX: Graphics primitives
 * - WiFi: ESP32 WiFi
 * - PubSubClient: MQTT client
 * - ArduinoJson: JSON parsing
 */

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "../version.h"
#include "../config.h"
#include "eink_display.h"
#include "screen_base.h"

// ===== MQTT CLIENT =====
WiFiClient espClient;
PubSubClient mqttClient(espClient);

// ===== CONNECTION STATE =====
bool wifi_connected = false;
bool mqtt_connected = false;

// ===== SCREEN ENUM =====
typedef enum {
    SCREEN_SPLASH,
    SCREEN_GRID_POWER,
    SCREEN_COUNT  // Total number of screens
} screen_id_t;

// ===== SCREEN FUNCTION DECLARATIONS =====
// Splash Screen
void ui_Screen_Splash_init(void);
void ui_Screen_Splash_destroy(void);
void ui_Screen_Splash_set_status(const char* status);
lv_obj_t* ui_Screen_Splash_get_obj(void);

// Grid Power Screen
void ui_Screen_GridPower_init(void);
void ui_Screen_GridPower_destroy(void);
void ui_Screen_GridPower_on_activate(void);
void ui_Screen_GridPower_on_deactivate(void);
void ui_Screen_GridPower_on_mqtt_message(const char* topic, const char* payload);
lv_obj_t* ui_Screen_GridPower_get_obj(void);

// ===== SCREEN REGISTRY =====
// Array of all screens with their lifecycle functions
// This table drives the screen manager
screen_t screens[SCREEN_COUNT] = {
    // Splash Screen (shows connection status)
    {
        .init = ui_Screen_Splash_init,
        .destroy = ui_Screen_Splash_destroy,
        .on_activate = NULL,
        .on_deactivate = NULL,
        .on_mqtt_message = NULL,
        .on_update = NULL,
        .screen_obj = NULL
    },
    // Grid Power Screen (MQTT-driven)
    {
        .init = ui_Screen_GridPower_init,
        .destroy = ui_Screen_GridPower_destroy,
        .on_activate = ui_Screen_GridPower_on_activate,
        .on_deactivate = ui_Screen_GridPower_on_deactivate,
        .on_mqtt_message = ui_Screen_GridPower_on_mqtt_message,
        .on_update = NULL,
        .screen_obj = NULL
    }
};

// ===== SCREEN MANAGER STATE =====
screen_id_t current_screen_id = SCREEN_SPLASH;
unsigned long screen_start_time = 0;

// ===== DISPLAY REFRESH STATE =====
bool display_needs_refresh = false;

// ===== WIFI CONNECTION =====
void connect_wifi() {
    Serial.print("\n[WiFi] Connecting to ");
    Serial.println(WIFI_SSID);
    ui_Screen_Splash_set_status("Connecting to WiFi...");
    lv_refr_now(eink_get_display());
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
        wifi_connected = true;
        Serial.println("\n[WiFi] Connected!");
        Serial.print("[WiFi] IP address: ");
        Serial.println(WiFi.localIP());
        ui_Screen_Splash_set_status("WiFi connected");
        lv_refr_now(eink_get_display());
    } else {
        Serial.println("\n[WiFi] Failed to connect");
        ui_Screen_Splash_set_status("WiFi failed");
        lv_refr_now(eink_get_display());
    }
}

// ===== MQTT CALLBACK =====
void mqtt_callback(char* topic, byte* payload, unsigned int length) {
    // Convert payload to string
    char message[length + 1];
    memcpy(message, payload, length);
    message[length] = '\0';
    
    Serial.print("[MQTT] Message arrived on topic: ");
    Serial.println(topic);
    
    // Route message to current screen's MQTT handler
    screen_t *current_screen = &screens[current_screen_id];
    if (current_screen->on_mqtt_message) {
        current_screen->on_mqtt_message(topic, message);
    }
}

// ===== MQTT CONNECTION =====
void connect_mqtt() {
    Serial.println("\n[MQTT] Connecting to broker...");
    ui_Screen_Splash_set_status("Connecting to MQTT...");
    lv_refr_now(eink_get_display());
    
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setBufferSize(512);  // Increase buffer for larger JSON messages
    mqttClient.setCallback(mqtt_callback);
    
    Serial.print("[MQTT] Buffer size set to: ");
    Serial.println(mqttClient.getBufferSize());
    
    // Generate client ID
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    
    int attempts = 0;
    while (!mqttClient.connected() && attempts < 5) {
        Serial.print("[MQTT] Attempting connection... ");
        
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            mqtt_connected = true;
            Serial.println("connected");
            ui_Screen_Splash_set_status("MQTT connected");
            lv_refr_now(eink_get_display());
            
            // Subscribe to topics
            Serial.print("[MQTT] Subscribing to: ");
            Serial.println(MQTT_TOPIC_GRID_POWER);
            bool subscribed = mqttClient.subscribe(MQTT_TOPIC_GRID_POWER);
            if (subscribed) {
                Serial.println("[MQTT] Grid power subscription successful");
            } else {
                Serial.println("[MQTT] Grid power subscription failed!");
            }
            
            Serial.print("[MQTT] Subscribing to: ");
            Serial.println(MQTT_TOPIC_SOLAR_POWER);
            subscribed = mqttClient.subscribe(MQTT_TOPIC_SOLAR_POWER);
            if (subscribed) {
                Serial.println("[MQTT] Solar power subscription successful");
            } else {
                Serial.println("[MQTT] Solar power subscription failed!");
            }
        } else {
            Serial.print("failed, rc=");
            Serial.print(mqttClient.state());
            Serial.println(" retrying in 2 seconds");
            attempts++;
            delay(2000);
        }
    }
    
    if (!mqtt_connected) {
        Serial.println("[MQTT] Failed to connect");
        ui_Screen_Splash_set_status("MQTT failed");
        lv_refr_now(eink_get_display());
    }
}

// ===== MQTT RECONNECTION =====
void reconnect_mqtt() {
    if (!mqttClient.connected()) {
        Serial.println("[MQTT] Connection lost, reconnecting...");
        mqtt_connected = false;
        
        String clientId = "ESP32-";
        clientId += String(random(0xffff), HEX);
        
        if (mqttClient.connect(clientId.c_str(), MQTT_USER, MQTT_PASSWORD)) {
            Serial.println("[MQTT] Reconnected");
            mqtt_connected = true;
            
            // Resubscribe to topics
            Serial.print("[MQTT] Resubscribing to: ");
            Serial.println(MQTT_TOPIC_GRID_POWER);
            mqttClient.subscribe(MQTT_TOPIC_GRID_POWER);
            Serial.print("[MQTT] Resubscribing to: ");
            Serial.println(MQTT_TOPIC_SOLAR_POWER);
            mqttClient.subscribe(MQTT_TOPIC_SOLAR_POWER);
        }
    }
}

// ===== DISPLAY REFRESH TRIGGER =====
void trigger_display_refresh() {
    display_needs_refresh = true;
}

// ===== SCREEN MANAGEMENT =====
// Load a screen by ID
// 1. Deactivate current screen (unsubscribe MQTT topics)
// 2. Destroy current screen (free LVGL objects)
// 3. Initialize new screen (create LVGL objects)
// 4. Activate new screen (subscribe MQTT topics)
// 5. Load screen into LVGL display
void load_screen(screen_id_t screen_id)
{
    screen_t *old_screen = &screens[current_screen_id];
    screen_t *new_screen = &screens[screen_id];
    
    // Don't destroy if loading the same screen for the first time
    bool is_different_screen = (current_screen_id != screen_id) || (old_screen->screen_obj != NULL);
    
    // Deactivate old screen (unsubscribe MQTT)
    if(is_different_screen && old_screen->on_deactivate) {
        old_screen->on_deactivate();
    }
    
    // Initialize new screen (create LVGL objects)
    if(new_screen->init) {
        new_screen->init();
    }
    
    // Update screen_obj pointer
    if(screen_id == SCREEN_SPLASH) {
        new_screen->screen_obj = ui_Screen_Splash_get_obj();
    } else if(screen_id == SCREEN_GRID_POWER) {
        new_screen->screen_obj = ui_Screen_GridPower_get_obj();
    }
    
    // Load screen into LVGL (make new screen active BEFORE destroying old)
    if(new_screen->screen_obj) {
        lv_screen_load(new_screen->screen_obj);
    }
    
    // Destroy old screen AFTER new screen is active (avoids LVGL warning)
    // Only if transitioning to a different screen OR reloading an existing screen
    if(is_different_screen && old_screen->destroy) {
        old_screen->destroy();
    }
    
    // Activate new screen (subscribe MQTT)
    if(new_screen->on_activate) {
        new_screen->on_activate();
    }
    
    // Update state
    current_screen_id = screen_id;
    screen_start_time = millis();
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Print startup message
  Serial.println("\n=== ESP32 E-INK + LVGL + MQTT Demo ===");
  printVersionInfo();
  Serial.println();
  Serial.print("Chip Model: ");
  Serial.println(ESP.getChipModel());
  Serial.print("Chip Revision: ");
  Serial.println(ESP.getChipRevision());
  Serial.print("CPU Frequency: ");
  Serial.print(ESP.getCpuFreqMHz());
  Serial.println(" MHz");
  Serial.print("Flash Size: ");
  Serial.print(ESP.getFlashChipSize() / (1024 * 1024));
  Serial.println(" MB");
  Serial.println("===============================================\n");
  
  // Initialize e-ink display and LVGL
  eink_init();
  
  // Load splash screen
  load_screen(SCREEN_SPLASH);
  
  // Initial render
  Serial.println("[LVGL] Performing initial render...");
  lv_refr_now(eink_get_display());
  
  // Connect to WiFi
  connect_wifi();
  
  // Connect to MQTT if WiFi connected
  if (wifi_connected) {
    delay(1000);
    connect_mqtt();
  }
  
  // Transition to grid power screen if connected
  if (wifi_connected && mqtt_connected) {
    delay(2000);
    Serial.println("\n[APP] Connection successful - loading Grid Power screen");
    ui_Screen_Splash_set_status("Ready!");
    lv_refr_now(eink_get_display());
    delay(1000);
    load_screen(SCREEN_GRID_POWER);
    lv_refr_now(eink_get_display());
  } else {
    Serial.println("\n[APP] Connection failed - staying on splash screen");
  }
  
  Serial.println("\n[READY] System initialized");
  Serial.print("[INFO] Full refresh every ");
  Serial.print(MAX_PARTIAL_UPDATES);
  Serial.println(" e-ink flushes to clear ghosting\n");
  
  screen_start_time = millis();
}

void loop()
{
  static unsigned long lastHeartbeat = 0;
  unsigned long currentMillis = millis();
  
  // ===== MQTT CONNECTION MAINTENANCE =====
  if (wifi_connected && mqtt_connected) {
    if (!mqttClient.connected()) {
      reconnect_mqtt();
    }
    mqttClient.loop();
    
    // Heartbeat every 10 seconds
    if (currentMillis - lastHeartbeat >= 10000) {
      Serial.println("[MQTT] Heartbeat - still connected");
      lastHeartbeat = currentMillis;
    }
  }
  
  // ===== DISPLAY REFRESH =====
  // Refresh display when triggered by MQTT message
  if (display_needs_refresh) {
    Serial.println("[APP] Refreshing display...");
    
    // Check if we need a full refresh to clear ghosting
    eink_force_full_refresh();
    
    // Trigger LVGL refresh
    lv_refr_now(eink_get_display());
    
    // Power off panel voltages to prevent fading
    eink_poweroff();
    
    display_needs_refresh = false;
  }
  
  delay(10); // Small delay to prevent watchdog issues
}