/*
 * Home Screen (Hello LVGL)
 * 
 * Main application screen showing:
 * - Title and chip information
 * - Counter that updates periodically
 * - Progress bar cycling 0-10
 * 
 * LIFECYCLE:
 * - Shown after splash screen
 * - Updates every 2.5 seconds
 * - No MQTT subscriptions (yet)
 * - Stays on forever (until screen transition implemented)
 */

#include "screen_base.h"

// SquareLine Studio compatible globals
lv_obj_t *ui_Screen_Home = NULL;
lv_obj_t *ui_Label_Hello = NULL;
lv_obj_t *ui_Label_Chip = NULL;
lv_obj_t *ui_Label_Counter = NULL;
lv_obj_t *ui_ProgressBar = NULL;

// Screen-local state
static uint32_t update_counter = 0;

// Styles (must be static to persist after init)
static lv_style_t style_bg;
static lv_style_t style_indic;

// Screen initialization (SquareLine Studio pattern)
void ui_Screen_Home_init(void)
{
    Serial.println("[SCREEN] Creating Home screen...");
    
    // Create screen
    ui_Screen_Home = lv_obj_create(NULL);
    lv_obj_clear_flag(ui_Screen_Home, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(ui_Screen_Home, lv_color_white(), 0);
    
    // Title label
    ui_Label_Hello = lv_label_create(ui_Screen_Home);
    lv_label_set_text(ui_Label_Hello, "Hello LVGL!");
    lv_obj_set_style_text_color(ui_Label_Hello, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Hello, &lv_font_montserrat_32, 0);
    lv_obj_align(ui_Label_Hello, LV_ALIGN_TOP_MID, 0, 20);
    
    // Chip info label
    ui_Label_Chip = lv_label_create(ui_Screen_Home);
    String chip_info = String(ESP.getChipModel()) + "\n" + 
                       String(ESP.getCpuFreqMHz()) + " MHz";
    lv_label_set_text(ui_Label_Chip, chip_info.c_str());
    lv_obj_set_style_text_color(ui_Label_Chip, lv_color_black(), 0);
    lv_obj_set_style_text_align(ui_Label_Chip, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(ui_Label_Chip, LV_ALIGN_CENTER, 0, -20);
    
    // Counter label
    ui_Label_Counter = lv_label_create(ui_Screen_Home);
    lv_label_set_text(ui_Label_Counter, "Counter: 0");
    lv_obj_set_style_text_color(ui_Label_Counter, lv_color_black(), 0);
    lv_obj_set_style_text_font(ui_Label_Counter, &lv_font_montserrat_24, 0);
    lv_obj_align(ui_Label_Counter, LV_ALIGN_BOTTOM_MID, 0, -60);
    
    // Progress bar styles
    lv_style_init(&style_bg);
    lv_style_set_border_color(&style_bg, lv_color_black());
    lv_style_set_border_width(&style_bg, 2);
    lv_style_set_pad_all(&style_bg, 4);
    lv_style_set_radius(&style_bg, 4);
    lv_style_set_bg_color(&style_bg, lv_color_white());
    
    lv_style_init(&style_indic);
    lv_style_set_bg_opa(&style_indic, LV_OPA_COVER);
    lv_style_set_bg_color(&style_indic, lv_color_black());
    lv_style_set_radius(&style_indic, 2);
    
    // Progress bar
    ui_ProgressBar = lv_bar_create(ui_Screen_Home);
    lv_obj_remove_style_all(ui_ProgressBar);
    lv_obj_add_style(ui_ProgressBar, &style_bg, 0);
    lv_obj_add_style(ui_ProgressBar, &style_indic, LV_PART_INDICATOR);
    lv_obj_set_size(ui_ProgressBar, 180, 20);
    lv_obj_align(ui_ProgressBar, LV_ALIGN_BOTTOM_MID, 0, -20);
    lv_bar_set_range(ui_ProgressBar, 0, 10);
    lv_bar_set_value(ui_ProgressBar, 0, LV_ANIM_OFF);
    
    // Reset counter when screen is created
    update_counter = 0;
    
    Serial.println("[SCREEN] Home screen created");
}

// Screen destruction (SquareLine Studio pattern)
void ui_Screen_Home_destroy(void)
{
    Serial.println("[SCREEN] Destroying Home screen");
    
    if(ui_Screen_Home) {
        lv_obj_del(ui_Screen_Home);
    }
    
    // NULL all screen variables
    ui_Screen_Home = NULL;
    ui_Label_Hello = NULL;
    ui_Label_Chip = NULL;
    ui_Label_Counter = NULL;
    ui_ProgressBar = NULL;
}

// Periodic update (called from app.ino loop)
void ui_Screen_Home_on_update(void)
{
    update_counter++;
    
    // Use modulo to cycle 0-10
    uint32_t bar_value = update_counter % 11;
    
    // Update counter label
    String counter_text = "Counter: " + String(update_counter);
    lv_label_set_text(ui_Label_Counter, counter_text.c_str());
    
    // Update progress bar
    lv_bar_set_value(ui_ProgressBar, bar_value, LV_ANIM_OFF);
    
    Serial.print("[UPDATE] Home screen refresh #");
    Serial.print(update_counter);
    Serial.print(" | Free Heap: ");
    Serial.print(ESP.getFreeHeap());
    Serial.println(" bytes");
}

// Get screen object (for screen manager)
lv_obj_t* ui_Screen_Home_get_obj(void)
{
    return ui_Screen_Home;
}
