/*
 * Splash Screen
 * 
 * Splash screen shown at startup.
 * Displays project name, version, and connection status.
 * Connects to WiFi and MQTT during initialization.
 * 
 * LIFECYCLE:
 * - Shown on boot
 * - Connects to WiFi and MQTT
 * - Shows connection status
 * - Transitions to Grid Power screen when connected
 */

#include "screen_base.h"
#include "../version.h"

// SquareLine Studio compatible globals
lv_obj_t *ui_Screen_Splash = NULL;
lv_obj_t *ui_Label_Splash_Title = NULL;
lv_obj_t *ui_Label_Splash_Version = NULL;
lv_obj_t *ui_Label_Splash_Status = NULL;

// Screen initialization (SquareLine Studio pattern)
void ui_Screen_Splash_init(void)
{
    // Create the screen
    ui_Screen_Splash = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Splash, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_Splash, lv_color_white(), 0);
    
    // Title label
    ui_Label_Splash_Title = lv_label_create(ui_Screen_Splash);
    lv_label_set_text(ui_Label_Splash_Title, "ESP32 E-INK");
    lv_obj_set_style_text_color(ui_Label_Splash_Title, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Splash_Title, &lv_font_montserrat_32, 0);
    lv_obj_align(ui_Label_Splash_Title, LV_ALIGN_CENTER, 0, -40);
    
    // Version label
    ui_Label_Splash_Version = lv_label_create(ui_Screen_Splash);
    String version_text = String("v") + String(VERSION_MAJOR) + "." + 
                         String(VERSION_MINOR) + "." + String(VERSION_PATCH);
    lv_label_set_text(ui_Label_Splash_Version, version_text.c_str());
    lv_obj_set_style_text_color(ui_Label_Splash_Version, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Splash_Version, &lv_font_montserrat_14, 0);
    lv_obj_align(ui_Label_Splash_Version, LV_ALIGN_CENTER, 0, 0);
    
    // Status label
    ui_Label_Splash_Status = lv_label_create(ui_Screen_Splash);
    lv_label_set_text(ui_Label_Splash_Status, "Initializing...");
    lv_obj_set_style_text_color(ui_Label_Splash_Status, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Splash_Status, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(ui_Label_Splash_Status, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui_Label_Splash_Status, LV_ALIGN_BOTTOM_MID, 0, -30);
}

// Screen destruction (SquareLine Studio pattern)
void ui_Screen_Splash_destroy(void)
{
    if(ui_Screen_Splash) {
        lv_obj_del(ui_Screen_Splash);
    }
    
    // NULL all screen variables
    ui_Screen_Splash = NULL;
    ui_Label_Splash_Title = NULL;
    ui_Label_Splash_Version = NULL;
    ui_Label_Splash_Status = NULL;
}

// Get screen object (for screen manager)
lv_obj_t* ui_Screen_Splash_get_obj(void)
{
    return ui_Screen_Splash;
}

// Update status message
void ui_Screen_Splash_set_status(const char* status)
{
    if(ui_Label_Splash_Status) {
        lv_label_set_text(ui_Label_Splash_Status, status);
    }
}
