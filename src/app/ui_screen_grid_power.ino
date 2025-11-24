/*
 * Grid Power Screen
 * 
 * Displays real-time power data from MQTT topics:
 * - Grid power: data/devices/utility_meter/total_active_power (JSON)
 * - Solar power: shelly_solar_power (plain float)
 * 
 * Grid power JSON format:
 * {
 *   "value": 2.56,
 *   "unit": "kW",
 *   "timestamp": 1763905637,
 *   "friendly_name": "Total active power",
 *   "sensorId": "utility_meter.total_active_power",
 *   "entity": "Utility meter",
 *   "metric": "GridElectricityPower",
 *   "metricKind": "gauge"
 * }
 * 
 * Solar power format: "0.02471"
 * 
 * LIFECYCLE:
 * - Subscribes to MQTT topics when activated
 * - Updates display when MQTT messages received
 * - Unsubscribes when deactivated
 */

#include "screen_base.h"
#include "../config.h"
#include "power_icons.h"
#include <ArduinoJson.h>

// SquareLine Studio compatible globals
lv_obj_t *ui_Screen_GridPower = NULL;

// Value labels (below icons)
lv_obj_t *ui_Label_Grid_Value = NULL;
lv_obj_t *ui_Label_House_Value = NULL;
lv_obj_t *ui_Label_Solar_Value = NULL;

// Arrow labels (between icons)
lv_obj_t *ui_Label_Arrow_GridHome = NULL;   // Between Grid and Home
lv_obj_t *ui_Label_Arrow_HomeSolar = NULL;  // Between Home and Solar

// Current values
static float current_solar = 0.0;
static float current_grid = 0.0;

// Screen initialization
void ui_Screen_GridPower_init(void)
{
    Serial.println("[GridPower] Initializing screen");
    
    // Create the screen
    ui_Screen_GridPower = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_GridPower, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_GridPower, lv_color_white(), 0);
    
    // ===== CREATE CANVAS FOR ICON BACKGROUND =====
    Serial.println("[GridPower] Creating canvas for icons...");
    
    // Create canvas object
    lv_obj_t *canvas = lv_canvas_create(ui_Screen_GridPower);
    lv_obj_set_pos(canvas, 0, 0);
    
    // Use heap allocation to avoid DRAM overflow
    size_t buf_size = LV_CANVAS_BUF_SIZE(200, 200, 1, 1);
    uint8_t *canvas_buffer = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_8BIT);
    
    if (canvas_buffer == NULL) {
        Serial.println("[GridPower] ERROR: Failed to allocate canvas buffer!");
        return;
    }
    
    memset(canvas_buffer, 0xFF, buf_size);  // Fill with white
    lv_canvas_set_buffer(canvas, canvas_buffer, 200, 200, LV_COLOR_FORMAT_I1);
    
    // Set palette for I1 (indexed 1-bit) format
    lv_canvas_set_palette(canvas, 0, lv_color_to_32(lv_color_black(), LV_OPA_COVER));
    lv_canvas_set_palette(canvas, 1, lv_color_to_32(lv_color_white(), LV_OPA_COVER));
    
    // Draw icon pixels from PNG data
    extern const uint8_t power_icons_map[];
    const uint8_t *img_data = power_icons_map;
    int stride = 25;  // 200 pixels / 8 = 25 bytes per row
    
    Serial.println("[GridPower] Drawing icon pixels...");
    int pixels_drawn = 0;
    
    for (int y = 0; y < 200; y++) {
        for (int x = 0; x < 200; x++) {
            int byte_idx = y * stride + (x / 8);
            int bit_idx = 7 - (x % 8);
            bool is_white = (img_data[byte_idx] >> bit_idx) & 0x01;
            
            if (!is_white) {
                lv_canvas_set_px(canvas, x, y, lv_color_black(), LV_OPA_COVER);
                pixels_drawn++;
            }
        }
    }
    
    Serial.printf("[GridPower] Canvas created, pixels drawn: %d\n", pixels_drawn);
    
    // ===== LAYOUT: Grid (left) | Home (center) | Sun (right) in top row =====
    // Icons are in the PNG at top, positioned roughly:
    // Grid: x=15-50, Home: x=80-120, Sun: x=150-185
    // 35px gaps between icons for arrows
    
    // Arrow between Grid and Home (x position ~52-78)
    ui_Label_Arrow_GridHome = lv_label_create(ui_Screen_GridPower);
    lv_label_set_text(ui_Label_Arrow_GridHome, " ");  // Initially hidden
    lv_obj_set_style_text_color(ui_Label_Arrow_GridHome, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Arrow_GridHome, &lv_font_montserrat_32, 0);
    lv_obj_set_pos(ui_Label_Arrow_GridHome, 58, 10);
    
    // Arrow between Home and Solar (x position ~122-148)
    ui_Label_Arrow_HomeSolar = lv_label_create(ui_Screen_GridPower);
    lv_label_set_text(ui_Label_Arrow_HomeSolar, " ");  // Initially hidden
    lv_obj_set_style_text_color(ui_Label_Arrow_HomeSolar, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Arrow_HomeSolar, &lv_font_montserrat_32, 0);
    lv_obj_set_pos(ui_Label_Arrow_HomeSolar, 128, 10);
    
    // ===== VALUE LABELS (below icons) =====
    // Grid value (below left icon)
    ui_Label_Grid_Value = lv_label_create(ui_Screen_GridPower);
    lv_label_set_text(ui_Label_Grid_Value, "-- kW");
    lv_obj_set_style_text_color(ui_Label_Grid_Value, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Grid_Value, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ui_Label_Grid_Value, 5, 50);
    
    // House value (below center icon)
    ui_Label_House_Value = lv_label_create(ui_Screen_GridPower);
    lv_label_set_text(ui_Label_House_Value, "-- kW");
    lv_obj_set_style_text_color(ui_Label_House_Value, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_House_Value, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ui_Label_House_Value, 80, 50);
    
    // Solar value (below right icon)
    ui_Label_Solar_Value = lv_label_create(ui_Screen_GridPower);
    lv_label_set_text(ui_Label_Solar_Value, "-- kW");
    lv_obj_set_style_text_color(ui_Label_Solar_Value, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Solar_Value, &lv_font_montserrat_14, 0);
    lv_obj_set_pos(ui_Label_Solar_Value, 155, 50);
}

// Screen destruction
void ui_Screen_GridPower_destroy(void)
{
    Serial.println("[GridPower] Destroying screen");
    
    if(ui_Screen_GridPower) {
        lv_obj_del(ui_Screen_GridPower);
    }
    
    // NULL all screen variables
    ui_Screen_GridPower = NULL;
    ui_Label_Grid_Value = NULL;
    ui_Label_House_Value = NULL;
    ui_Label_Solar_Value = NULL;
    ui_Label_Arrow_GridHome = NULL;
    ui_Label_Arrow_HomeSolar = NULL;
}

// Get screen object (for screen manager)
lv_obj_t* ui_Screen_GridPower_get_obj(void)
{
    return ui_Screen_GridPower;
}

// Called when screen becomes active (subscribe to MQTT)
void ui_Screen_GridPower_on_activate(void)
{
    Serial.println("[GridPower] Screen activated - subscribing to MQTT topics");
    // MQTT subscription handled by app.ino
}

// Called when leaving screen (unsubscribe from MQTT)
void ui_Screen_GridPower_on_deactivate(void)
{
    Serial.println("[GridPower] Screen deactivated - unsubscribing from MQTT topics");
    // MQTT unsubscription handled by app.ino
}

// Update display with current values and arrow logic
void update_power_display()
{
    // Calculate house consumption
    float house_power = current_solar + current_grid;
    
    // Update value labels (all in kW)
    char gridStr[16], houseStr[16], solarStr[16];
    snprintf(gridStr, sizeof(gridStr), "%.2f kW", fabs(current_grid));
    snprintf(houseStr, sizeof(houseStr), "%.2f kW", house_power);
    snprintf(solarStr, sizeof(solarStr), "%.2f kW", current_solar);
    
    lv_label_set_text(ui_Label_Grid_Value, gridStr);
    lv_label_set_text(ui_Label_House_Value, houseStr);
    lv_label_set_text(ui_Label_Solar_Value, solarStr);
    
    // ===== ARROW LOGIC (simplified) =====
    // Layout: Grid | Home | Solar (left to right)
    // Arrow between Grid-Home, Arrow between Home-Solar
    
    const float threshold = 0.01;  // 10W threshold
    
    // Arrow between Grid and Home
    if (current_grid > threshold) {
        // Importing from grid: G > H
        lv_label_set_text(ui_Label_Arrow_GridHome, ">");
    } else if (current_grid < -threshold) {
        // Exporting to grid: G < H
        lv_label_set_text(ui_Label_Arrow_GridHome, "<");
    } else {
        // No significant flow
        lv_label_set_text(ui_Label_Arrow_GridHome, " ");
    }
    
    // Arrow between Home and Solar
    if (current_solar > threshold) {
        // Solar producing, flows to home: H < S
        lv_label_set_text(ui_Label_Arrow_HomeSolar, "<");
    } else {
        // No solar production
        lv_label_set_text(ui_Label_Arrow_HomeSolar, " ");
    }
    
    Serial.print("[GridPower] Power flow: Solar=");
    Serial.print(current_solar);
    Serial.print("kW + Grid=");
    Serial.print(current_grid);
    Serial.print("kW = House=");
    Serial.print(house_power);
    Serial.println("kW");
}

// Handle MQTT messages
void ui_Screen_GridPower_on_mqtt_message(const char* topic, const char* payload)
{
    Serial.print("[GridPower] MQTT message received on topic: ");
    Serial.println(topic);
    Serial.print("[GridPower] Payload: ");
    Serial.println(payload);
    
    bool updated = false;
    
    // Check which topic this is
    if (strcmp(topic, MQTT_TOPIC_GRID_POWER) == 0) {
        // Parse JSON for grid power
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, payload);
        
        if (error) {
            Serial.print("[GridPower] JSON parse error: ");
            Serial.println(error.c_str());
            return;
        }
        
        // Extract grid power value
        current_grid = doc["value"] | 0.0;
        updated = true;
    }
    else if (strcmp(topic, MQTT_TOPIC_SOLAR_POWER) == 0) {
        // Parse plain float value for solar power
        current_solar = atof(payload);
        updated = true;
    }
    
    if (updated) {
        // Update entire display with new calculation
        update_power_display();
        
        // Trigger display refresh
        extern void trigger_display_refresh();
        trigger_display_refresh();
    }
}
