/*
 * Splash Screen
 * 
 * Simple splash screen shown at startup for 5 seconds.
 * Displays project name and version information.
 * 
 * LIFECYCLE:
 * - Shown on boot for 5 seconds
 * - No MQTT subscriptions
 * - No periodic updates
 * - Automatically transitions to Home screen
 */

#include "screen_base.h"
#include "../version.h"

// SquareLine Studio compatible globals
lv_obj_t *ui_Screen_Splash = NULL;
lv_obj_t *ui_Label_Splash_Title = NULL;
lv_obj_t *ui_Label_Splash_Version = NULL;
lv_obj_t *ui_Label_Splash_Chip = NULL;

// Screen initialization (SquareLine Studio pattern)
void ui_Screen_Splash_init(void)
{
    Serial.println("[SCREEN] Creating Splash screen...");
    
    // Create screen
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
    
    // Chip info label
    ui_Label_Splash_Chip = lv_label_create(ui_Screen_Splash);
    String chip_info = String(ESP.getChipModel()) + " @ " + 
                      String(ESP.getCpuFreqMHz()) + " MHz";
    lv_label_set_text(ui_Label_Splash_Chip, chip_info.c_str());
    lv_obj_set_style_text_color(ui_Label_Splash_Chip, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Splash_Chip, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_align(ui_Label_Splash_Chip, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui_Label_Splash_Chip, LV_ALIGN_BOTTOM_MID, 0, -40);
    
    Serial.println("[SCREEN] Splash screen created");
}

// Screen destruction (SquareLine Studio pattern)
void ui_Screen_Splash_destroy(void)
{
    Serial.println("[SCREEN] Destroying Splash screen");
    
    if(ui_Screen_Splash) {
        lv_obj_del(ui_Screen_Splash);
    }
    
    // NULL all screen variables
    ui_Screen_Splash = NULL;
    ui_Label_Splash_Title = NULL;
    ui_Label_Splash_Version = NULL;
    ui_Label_Splash_Chip = NULL;
}

// Get screen object (for screen manager)
lv_obj_t* ui_Screen_Splash_get_obj(void)
{
    return ui_Screen_Splash;
}
