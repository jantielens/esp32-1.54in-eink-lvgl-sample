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
 * - 0 = white, 1 = black (INVERTED from LVGL!)
 * - No header, just raw pixel data
 * 
 * REFRESH STRATEGY:
 * - Counts partial updates
 * - Forces full refresh every MAX_PARTIAL_UPDATES to clear ghosting
 * - Full refresh: hibernate → reinit → full window update
 * - Partial refresh: small window update (faster, less ghosting)
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
static uint8_t full_refresh_row_bitmap[(SCREEN_HEIGHT + 7) / 8] = {0};

static inline void set_full_frame_pixel(uint16_t x, uint16_t y, bool is_black);
static inline bool get_full_frame_pixel(uint16_t x, uint16_t y);
static void mark_rows_covered(uint16_t y1, uint16_t y2);
static bool all_rows_covered();
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

  display.setPartialWindow(area->x1, area->y1, w, h);
  
  // Increment counter after deciding
  // Note: This counts flush calls, not user update cycles
  // (LVGL may flush multiple dirty regions per user update)
  partial_update_count++;
  
  // ===== PIXEL FORMAT CONVERSION =====
  // We need to convert LVGL's pixel format to GxEPD2's expected format.
  // Both use 1 bit per pixel, but the bit meanings are INVERTED.
  //
  // Calculate stride (bytes per row):
  // - Width in bits: w
  // - Width in bytes: (w + 7) / 8  (rounds up)
  uint32_t epd_stride = (w + 7) / 8;
  uint32_t buffer_size = epd_stride * h;
  
  // Allocate temporary buffer for converted pixels
  uint8_t *epd_buffer = (uint8_t*)malloc(buffer_size);
  
  if (epd_buffer) {
    // Convert LVGL format to GxEPD2 format
    uint32_t lvgl_stride = (w + 7) / 8;
    
    // Process each row of pixels
    for (uint32_t y = 0; y < h; y++) {
      // Process each byte in the row (each byte contains 8 pixels)
      for (uint32_t byte_x = 0; byte_x < epd_stride; byte_x++) {
        uint8_t epd_byte = 0;  // Build output byte bit by bit
        
        // Process each bit (pixel) in the byte
        for (uint8_t bit = 0; bit < 8; bit++) {
          uint32_t x = byte_x * 8 + bit;  // Calculate actual pixel x coordinate
          
          if (x < w) {  // Only process pixels within width (last byte may be partial)
            // Find the pixel in LVGL buffer
            uint32_t lvgl_byte_idx = y * lvgl_stride + (x / 8);
            uint8_t lvgl_bit = 7 - (x % 8);  // MSB first
            uint8_t pixel = (px_map[lvgl_byte_idx] >> lvgl_bit) & 0x01;
            
            // INVERT pixel value:
            // LVGL: 0=black, 1=white
            // GxEPD2: 0=white, 1=black (inverted!)
            bool is_black = (pixel == 0);
            if (is_black) {
              epd_byte |= (0x80 >> bit);  // Set bit to 1 for GxEPD2
            }
            set_full_frame_pixel(area->x1 + x, area->y1 + y, is_black);
          }
        }
        // Store converted byte
        epd_buffer[y * epd_stride + byte_x] = epd_byte;
      }
    }
    
    display.firstPage();
    do {
      // Draw each pixel to ensure correct placement
      for (uint32_t y = 0; y < h; y++) {
        for (uint32_t byte_x = 0; byte_x < epd_stride; byte_x++) {
          uint8_t data_byte = epd_buffer[y * epd_stride + byte_x];
          
          for (uint8_t bit = 0; bit < 8; bit++) {
            uint32_t x = byte_x * 8 + bit;
            if (x < w) {
              bool is_black = (data_byte & (0x80 >> bit)) != 0;
              display.writePixel(area->x1 + x, area->y1 + y, is_black ? GxEPD_BLACK : GxEPD_WHITE);
            }
          }
        }
      }
    } while (display.nextPage());
    
    free(epd_buffer);
  } else {
    Serial.println("[ERROR] Failed to allocate EPD buffer!");
    display.firstPage();
    do {
      // Just clear on error
    } while (display.nextPage());
  }
  
  if (full_refresh_pending) {
    mark_rows_covered(area->y1, area->y2);
    if (all_rows_covered()) {
      perform_full_refresh();
    }
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
  display.init(115200, true, 2, false);
  display.setRotation(0);
  display.setTextColor(GxEPD_BLACK);
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
    full_frame_buffer[byte_index] |= bit_mask;
  } else {
    full_frame_buffer[byte_index] &= ~bit_mask;
  }
}

// Read a pixel back from the framebuffer during the scheduled full refresh.
static inline bool get_full_frame_pixel(uint16_t x, uint16_t y)
{
  uint32_t bit_index = static_cast<uint32_t>(y) * SCREEN_WIDTH + x;
  uint32_t byte_index = bit_index / 8;
  uint8_t bit_mask = 0x80 >> (bit_index % 8);
  return (full_frame_buffer[byte_index] & bit_mask) != 0;
}

// Track which rows LVGL has flushed so we know when the whole screen is dirtied.
static void mark_rows_covered(uint16_t y1, uint16_t y2)
{
  for (uint16_t row = y1; row <= y2 && row < SCREEN_HEIGHT; row++) {
    uint16_t bit_index = row;
    uint16_t byte_index = bit_index / 8;
    uint8_t bit_mask = 1u << (bit_index % 8);
    full_refresh_row_bitmap[byte_index] |= bit_mask;
  }
}

// Returns true once every row has been touched since the refresh was scheduled.
static bool all_rows_covered()
{
  const uint16_t total_rows = SCREEN_HEIGHT;
  const uint16_t full_bytes = total_rows / 8;
  for (uint16_t i = 0; i < full_bytes; i++) {
    if (full_refresh_row_bitmap[i] != 0xFF) {
      return false;
    }
  }
  uint16_t remaining_bits = total_rows % 8;
  if (remaining_bits == 0) {
    return true;
  }
  uint8_t mask = static_cast<uint8_t>((1u << remaining_bits) - 1u);
  return (full_refresh_row_bitmap[full_bytes] & mask) == mask;
}

// Replays the stored framebuffer after hibernating/reinitializing to clear ghosting.
static void perform_full_refresh()
{
  Serial.println("[E-INK] Executing scheduled full refresh");
  display.hibernate();
  display.init(115200, true, 2, false);
  display.setFullWindow();
  display.firstPage();
  do {
    for (uint16_t y = 0; y < SCREEN_HEIGHT; y++) {
      for (uint16_t x = 0; x < SCREEN_WIDTH; x++) {
        bool is_black = get_full_frame_pixel(x, y);
        display.writePixel(x, y, is_black ? GxEPD_BLACK : GxEPD_WHITE);
      }
    }
  } while (display.nextPage());
  partial_update_count = 0;
  full_refresh_pending = false;
  std::memset(full_refresh_row_bitmap, 0, sizeof(full_refresh_row_bitmap));
}

// Flag that the next sweep should perform a full refresh once all rows have flushed.
static void schedule_full_refresh_request()
{
  if (full_refresh_pending) {
    return;
  }
  full_refresh_pending = true;
  std::memset(full_refresh_row_bitmap, 0, sizeof(full_refresh_row_bitmap));
}
