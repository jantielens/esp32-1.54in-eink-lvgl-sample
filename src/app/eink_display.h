/*
 * E-ink Display Driver for LVGL
 * 
 * This file provides the integration layer between:
 * - GxEPD2 (hardware e-ink driver library)
 * - LVGL (graphics library)
 * - ESP32 (microcontroller)
 * 
 * KEY RESPONSIBILITIES:
 * - Initialize GxEPD2 display hardware
 * - Initialize LVGL with correct color format and buffers
 * - Implement LVGL flush callback (my_disp_flush)
 * - Convert LVGL pixel format to GxEPD2 format
 * - Manage partial vs full refresh strategy
 * 
 * MEMORY LAYOUT:
 * - LVGL uses I1 format (1 bit per pixel)
 * - 50-line buffer = 200 * 50 / 8 = 1,250 bytes per buffer
 * - Total: ~10KB for display buffers (vs 5KB full framebuffer)
 * - LVGL heap: 64KB (configurable in lv_conf.h)
 * 
 * REFRESH STRATEGY:
 * - Partial updates: Fast (0.5-2s) but cause ghosting
 * - Full refresh: Slow (2-5s) but clears ghosting
 * - Hybrid: N partial updates, then 1 full refresh
 * - Configurable via MAX_PARTIAL_UPDATES
 * 
 * HARDWARE:
 * - WeAct Studio 1.54" E-Paper Module (GDEH0154D67)
 * - Resolution: 200x200 pixels
 * - Controller: SSD1681
 * - Interface: 4-wire SPI
 */

#ifndef EINK_DISPLAY_H
#define EINK_DISPLAY_H

#include <lvgl.h>
#include <GxEPD2_BW.h>

// ===== E-INK DISPLAY CONFIGURATION =====
// ESP32 Pin Configuration
// These pins connect the e-ink display to ESP32 via SPI interface.
// The pin numbers are from WeAct Studio's reference design and
// are compatible with most ESP32 dev boards.
//
// IMPORTANT: Keep SPI wires short (<15cm) to minimize signal noise!
//
#define EINK_CS    5   // Chip Select (active LOW, selects display on SPI bus)
#define EINK_DC    16  // Data/Command (LOW=command, HIGH=data)
#define EINK_RST   17  // Reset (active LOW, has internal pull-up)
#define EINK_BUSY  4   // Busy signal (HIGH=display updating, LOW=ready)
// SPI: SCK=18, MOSI=23 (hardware SPI pins, auto-configured by GxEPD2)

// Display resolution
#define SCREEN_WIDTH  200
#define SCREEN_HEIGHT 200

// LVGL buffer size (10 lines for partial updates)
#define LVGL_BUFFER_SIZE (SCREEN_WIDTH * 10)

// ===== REFRESH CONTROL =====
// Full refresh after N partial e-ink updates (not user cycles!)
// Note: Multiple widgets may cause multiple flushes per cycle
// Example: With 2 widgets updating, 50 flushes = 25 user cycles
#define MAX_PARTIAL_UPDATES 50

// ===== DISPLAY FUNCTIONS =====
void eink_init();
void eink_force_full_refresh();
void eink_poweroff();
void eink_full_refresh_now();
lv_display_t* eink_get_display();

#endif // EINK_DISPLAY_H
