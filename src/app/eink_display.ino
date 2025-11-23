/*
 * E-ink Display Driver Implementation
 * 
 * This file contains the critical integration code that bridges
 * LVGL's rendering system with GxEPD2's e-ink hardware driver.
 * 
 * MOST IMPORTANT FUNCTION: my_disp_flush()
 * This is the LVGL display driver callback that:
 * 1. Receives rendered pixels from LVGL in I1 format
 * 2. Converts pixel format from LVGL to GxEPD2
 * 3. Manages partial vs full refresh strategy
 * 4. Writes pixels to e-ink display hardware
 * 
 * PIXEL FORMAT CONVERSION:
 * LVGL I1 format:
 * - 1 bit per pixel (8 pixels per byte)
 * - MSB first within each byte
 * - 0 = black, 1 = white
 * - Includes 8-byte palette header (we skip it)
 * 
 * GxEPD2 format:
 * - 1 bit per pixel (8 pixels per byte)
 * - MSB first within each byte
 * - 0 = black, 1 = white (matches the controller's expectation)
 * - No header, just raw pixel data
 * 
 * REFRESH STRATEGY:
 * - Counts partial updates and schedules full-frame waveforms every MAX_PARTIAL_UPDATES
 * - Full refresh: replay cached framebuffer via controller's full waveform path
 * - Partial refresh: stream only the LVGL dirtied rectangle via differential (fast) waveform
 * - Panel power is held for PANEL_SETTLE_MS after each refresh to mimic vendor timing
 */

#include "eink_display.h"
#include <cstring>

// ===== GLOBAL OBJECTS =====
static GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display(
  GxEPD2_154_D67(EINK_CS, EINK_DC, EINK_RST, EINK_BUSY)
);

static lv_display_t *lv_disp = nullptr;
// LVGL buffer: 25 lines - balance between memory usage and flush efficiency
// Note: Widgets spanning >25px vertically will trigger multiple flushes, which is acceptable
static lv_color_t lv_disp_buf[SCREEN_WIDTH * 25] __attribute__((aligned(4)));
static uint8_t partial_update_count = 0;
static uint8_t full_frame_buffer[(SCREEN_WIDTH * SCREEN_HEIGHT + 7) / 8] = {0};
static bool full_refresh_pending = false;
static uint32_t last_epd_refresh_ms = 0;

static constexpr uint16_t PANEL_SETTLE_MS = 300;

static inline void set_full_frame_pixel(uint16_t x, uint16_t y, bool is_black);
static void push_full_frame_to_display();
static void push_partial_frame_to_display(const uint8_t *buffer, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
static void perform_full_refresh();
static void schedule_full_refresh_request();

// ===== LVGL DISPLAY DRIVER =====
// This is the heart of the driver - called by LVGL whenever the screen needs updating.
//
// PARAMETERS:
// @param disp    - LVGL display object
// @param area    - Rectangle to update (x1,y1 to x2,y2)
// @param px_map  - Pixel data in LVGL I1 format
//
// LVGL I1 FORMAT DETAILS:
// - First 8 bytes are palette (2 colors × 4 bytes each)
// - Actual pixel data starts at byte 8
// - Each bit represents one pixel: 0=color[0], 1=color[1]
// - In our config: 0=black, 1=white
//
// PROCESS:
// 1. Skip 8-byte palette header
// 2. Determine if this is a full-screen update
// 3. Decide between partial/full refresh based on counter
// 4. Convert pixel format (LVGL → GxEPD2)
// 5. Write to e-ink display
// 6. Signal LVGL that flush is complete
static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
  // CRITICAL: LVGL I1 format includes 8-byte palette header.
  // We must skip it to get to actual pixel data!
  px_map += 8;
  
  uint32_t w = area->x2 - area->x1 + 1;
  uint32_t h = area->y2 - area->y1 + 1;

  bool area_is_full_screen = (area->x1 <= 0) && (area->y1 <= 0) &&
                             (area->x2 >= (SCREEN_WIDTH - 1)) &&
                             (area->y2 >= (SCREEN_HEIGHT - 1));

  uint32_t epd_stride = (w + 7) / 8;
  uint32_t buffer_size = epd_stride * h;
  uint8_t *epd_buffer = (uint8_t*)malloc(buffer_size);

  if (epd_buffer) {
    uint32_t lvgl_stride = (w + 7) / 8;

    for (uint32_t y = 0; y < h; y++) {
      for (uint32_t byte_x = 0; byte_x < epd_stride; byte_x++) {
        uint8_t epd_byte = 0;
        for (uint8_t bit = 0; bit < 8; bit++) {
          uint32_t x = byte_x * 8 + bit;
          if (x < w) {
            uint32_t lvgl_byte_idx = y * lvgl_stride + (x / 8);
            uint8_t lvgl_bit = 7 - (x % 8);
            uint8_t pixel = (px_map[lvgl_byte_idx] >> lvgl_bit) & 0x01;
            bool is_black = (pixel == 0);
            if (!is_black) {
              epd_byte |= (0x80 >> bit);
            }
            set_full_frame_pixel(area->x1 + x, area->y1 + y, is_black);
          }
        }
        epd_buffer[y * epd_stride + byte_x] = epd_byte;
      }
    }

    bool need_full_refresh = full_refresh_pending || area_is_full_screen;
    if (need_full_refresh) {
      push_full_frame_to_display();
      partial_update_count = 0;
      full_refresh_pending = false;
    } else {
      push_partial_frame_to_display(epd_buffer, area->x1, area->y1, w, h);
      partial_update_count++;
    }

    free(epd_buffer);
  } else {
    Serial.println("[ERROR] Failed to allocate EPD buffer!");
  }

  lv_display_flush_ready(disp);
}

// ===== PUBLIC FUNCTIONS =====
void eink_init()
{
  // Initialize e-ink hardware
  Serial.println("[E-INK] Initializing display...");
  // init(serial_diag_bitrate, initial_refresh, reset_duration, pulldown_rst_mode)
  // serial_diag_bitrate: Set to 115200 to enable GxEPD2 debug timing output, or 0 to disable
  display.init(115200, true, 50, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
  std::memset(full_frame_buffer, 0xFF, sizeof(full_frame_buffer));
  Serial.println("[E-INK] Display ready");
  
  // Initialize LVGL
  Serial.println("[LVGL] Initializing...");
  lv_init();
  
  // Create LVGL display
  lv_disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(lv_disp, my_disp_flush);
  lv_display_set_buffers(lv_disp, lv_disp_buf, NULL, sizeof(lv_disp_buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_color_format(lv_disp, LV_COLOR_FORMAT_I1);
  
  Serial.println("[LVGL] Display created");
}

void eink_force_full_refresh()
{
  // Force full screen invalidation when threshold reached
  // Note: Counter tracks flush calls - with multiple widgets updating,
  // the threshold may be reached in fewer user update cycles than expected
  if (full_refresh_pending) {
    return;
  }
  if (partial_update_count >= MAX_PARTIAL_UPDATES) {
    schedule_full_refresh_request();
    lv_obj_invalidate(lv_screen_active());
  }
}

void eink_poweroff()
{
  // Turn off panel driving voltages to preserve image (avoids fading)
  // This is the proper way to keep image stable when power is removed
  uint32_t now = millis();
  uint32_t elapsed = now - last_epd_refresh_ms;
  if (elapsed < PANEL_SETTLE_MS) {
    delay(PANEL_SETTLE_MS - elapsed);
  }
  display.powerOff();
  Serial.println("[E-INK] Display powered off (voltages off, avoids fading)");
}

lv_display_t* eink_get_display()
{
  return lv_disp;
}

// Persist a single pixel into the 1-bit framebuffer so we can replay it later.
static inline void set_full_frame_pixel(uint16_t x, uint16_t y, bool is_black)
{
  uint32_t bit_index = static_cast<uint32_t>(y) * SCREEN_WIDTH + x;
  uint32_t byte_index = bit_index / 8;
  uint8_t bit_mask = 0x80 >> (bit_index % 8);
  if (is_black) {
    full_frame_buffer[byte_index] &= ~bit_mask;
  } else {
    full_frame_buffer[byte_index] |= bit_mask;
  }
}

// Read a pixel back from the framebuffer during the scheduled full refresh.
static void push_full_frame_to_display()
{
  display.epd2.writeImageForFullRefresh(full_frame_buffer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  display.epd2.refresh(false);
  if (display.epd2.hasFastPartialUpdate) {
    display.epd2.writeImageAgain(full_frame_buffer, 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
  }
  last_epd_refresh_ms = millis();
  display.powerOff();
}

static void push_partial_frame_to_display(const uint8_t *buffer, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
  display.epd2.writeImage(buffer, x, y, w, h);
  display.epd2.refresh(x, y, w, h);
  if (display.epd2.hasFastPartialUpdate) {
    display.epd2.writeImageAgain(buffer, x, y, w, h);
  }
  last_epd_refresh_ms = millis();
}

// Replays the stored framebuffer with a full refresh to clear ghosting.
// Uses display.refresh() which performs full waveform sequence (not partial).
static void perform_full_refresh()
{
  Serial.println("[E-INK] Executing scheduled full refresh");
  push_full_frame_to_display();
  partial_update_count = 0;
  full_refresh_pending = false;
}

// Flag that the next sweep should perform a full refresh once all rows have flushed.
static void schedule_full_refresh_request()
{
  if (full_refresh_pending) {
    return;
  }
  full_refresh_pending = true;
}

void eink_full_refresh_now()
{
  perform_full_refresh();
}
