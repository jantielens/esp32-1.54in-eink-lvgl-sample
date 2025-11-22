/*
 * ESP32 + E-Paper Display + LVGL Sample Project
 * 
 * This sample demonstrates best practices for integrating e-ink displays
 * with LVGL (Light and Versatile Graphics Library) on ESP32.
 * 
 * KEY CONCEPTS:
 * - E-ink displays are bistable (retain image without power)
 * - Slow refresh rates (2-5s full, 0.5-2s partial)
 * - Partial updates are faster but cause ghosting over time
 * - Manual refresh control saves power vs continuous polling
 * 
 * WHAT THIS DEMO SHOWS:
 * - Multi-screen architecture with lifecycle management
 * - Splash screen (5 seconds) -> Home screen (forever)
 * - SquareLine Studio compatible screen pattern
 * - MQTT-ready architecture (lifecycle hooks for future)
 * - Hybrid refresh strategy (partial + periodic full refresh)
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
 */

#include "../version.h"
#include "eink_display.h"
#include "screen_base.h"

// ===== SCREEN ENUM =====
typedef enum {
    SCREEN_SPLASH,
    SCREEN_HOME,
    SCREEN_COUNT  // Total number of screens
} screen_id_t;

// ===== SCREEN FUNCTION DECLARATIONS =====
// Splash Screen
void ui_Screen_Splash_init(void);
void ui_Screen_Splash_destroy(void);
lv_obj_t* ui_Screen_Splash_get_obj(void);

// Home Screen
void ui_Screen_Home_init(void);
void ui_Screen_Home_destroy(void);
void ui_Screen_Home_on_update(void);
lv_obj_t* ui_Screen_Home_get_obj(void);

// ===== SCREEN REGISTRY =====
// Array of all screens with their lifecycle functions
// This table drives the screen manager
screen_t screens[SCREEN_COUNT] = {
    // Splash Screen (5 seconds, no updates, no MQTT)
    {
        .init = ui_Screen_Splash_init,
        .destroy = ui_Screen_Splash_destroy,
        .on_activate = NULL,
        .on_deactivate = NULL,
        .on_mqtt_message = NULL,
        .on_update = NULL,
        .screen_obj = NULL
    },
    // Home Screen (periodic updates, no MQTT yet)
    {
        .init = ui_Screen_Home_init,
        .destroy = ui_Screen_Home_destroy,
        .on_activate = NULL,
        .on_deactivate = NULL,
        .on_mqtt_message = NULL,
        .on_update = ui_Screen_Home_on_update,
        .screen_obj = NULL
    }
};

// ===== SCREEN MANAGER STATE =====
screen_id_t current_screen_id = SCREEN_SPLASH;
unsigned long screen_start_time = 0;

// ===== APP STATE =====
unsigned long lastUpdate = 0;
const unsigned long UPDATE_INTERVAL = 2500; // Update every 2.5 seconds
const unsigned long SPLASH_DURATION = 5000; // Splash screen for 5 seconds

// ===== SCREEN MANAGEMENT =====
// Load a screen by ID
// 1. Deactivate current screen (unsubscribe MQTT topics)
// 2. Destroy current screen (free LVGL objects)
// 3. Initialize new screen (create LVGL objects)
// 4. Activate new screen (subscribe MQTT topics)
// 5. Load screen into LVGL display
void load_screen(screen_id_t screen_id)
{
    Serial.print("\n[SCREEN MGR] Transitioning from ");
    Serial.print(current_screen_id);
    Serial.print(" to ");
    Serial.println(screen_id);
    
    screen_t *old_screen = &screens[current_screen_id];
    screen_t *new_screen = &screens[screen_id];
    
    // Deactivate old screen (unsubscribe MQTT)
    if(old_screen->on_deactivate) {
        Serial.println("[SCREEN MGR] Deactivating old screen...");
        old_screen->on_deactivate();
    }
    
    // Destroy old screen (free LVGL objects)
    if(old_screen->destroy) {
        old_screen->destroy();
    }
    
    // Initialize new screen (create LVGL objects)
    if(new_screen->init) {
        new_screen->init();
    }
    
    // Update screen_obj pointer
    if(screen_id == SCREEN_SPLASH) {
        new_screen->screen_obj = ui_Screen_Splash_get_obj();
    } else if(screen_id == SCREEN_HOME) {
        new_screen->screen_obj = ui_Screen_Home_get_obj();
    }
    
    // Activate new screen (subscribe MQTT)
    if(new_screen->on_activate) {
        Serial.println("[SCREEN MGR] Activating new screen...");
        new_screen->on_activate();
    }
    
    // Load screen into LVGL
    if(new_screen->screen_obj) {
        lv_screen_load(new_screen->screen_obj);
        Serial.println("[SCREEN MGR] Screen loaded");
    }
    
    // Update state
    current_screen_id = screen_id;
    screen_start_time = millis();
    
    Serial.println("[SCREEN MGR] Transition complete\n");
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);
  delay(1000);
  
  // Print startup message
  Serial.println("\n=== ESP32 E-INK + LVGL Multi-Screen Demo ===");
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
  Serial.println("================================================\n");
  
  // Initialize e-ink display and LVGL
  eink_init();
  
  // Load initial screen (Splash)
  load_screen(SCREEN_SPLASH);
  
  // Initial render
  Serial.println("[LVGL] Performing initial render...");
  lv_refr_now(eink_get_display());
  
  Serial.println("\n[READY] System initialized");
  Serial.println("[INFO] Screen sequence: Splash (5s) -> Home (forever)");
  Serial.print("[INFO] Updates every ");
  Serial.print(UPDATE_INTERVAL / 1000.0);
  Serial.println(" seconds with partial refresh");
  Serial.print("[INFO] Full refresh every ");
  Serial.print(MAX_PARTIAL_UPDATES);
  Serial.println(" e-ink flushes to clear ghosting");
  Serial.println("[INFO] (Multiple widgets = multiple flushes per cycle)\n");
  
  lastUpdate = millis();
  screen_start_time = millis();
}

void loop()
{
  unsigned long currentMillis = millis();
  
  // ===== SCREEN TRANSITION LOGIC =====
  // Check if we need to transition from Splash to Home
  if (current_screen_id == SCREEN_SPLASH) {
    if (currentMillis - screen_start_time >= SPLASH_DURATION) {
      Serial.println("\n[APP] Splash timeout - transitioning to Home screen");
      load_screen(SCREEN_HOME);
      
      // Force full screen refresh after transition
      lv_refr_now(eink_get_display());
      
      lastUpdate = currentMillis;
    }
  }
  
  // ===== MANUAL REFRESH CONTROL =====
  // We control when the display updates (not automatic LVGL timer).
  // This is KEY for e-ink power efficiency:
  // - Only refresh when content changes
  // - Avoid continuous polling (lv_task_handler)
  // - Display retains image without power between updates
  //
  // For battery operation, you could use deep sleep between updates
  // and achieve <50µA average current!
  if (currentMillis - lastUpdate >= UPDATE_INTERVAL) {
    // Call current screen's update function (if it has one)
    screen_t *current_screen = &screens[current_screen_id];
    if (current_screen->on_update) {
      current_screen->on_update();
      
      // Check if we need a full refresh to clear ghosting
      eink_force_full_refresh();
      
      // Trigger LVGL refresh
      lv_refr_now(eink_get_display());
    }
    
    lastUpdate = currentMillis;
  }
  
  // LVGL task handler is NOT called here - we use manual refresh only
  // This is key for e-ink battery efficiency
  
  delay(100); // Small delay to prevent watchdog issues
}