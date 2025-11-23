/**
 * LVGL Configuration for ESP32 + E-ink Display
 * Optimized for 1.54" 200x200 monochrome e-paper
 */

#if 1 /*Set it to "1" to enable content*/

#ifndef LV_CONF_H
#define LV_CONF_H

/*====================
   COLOR SETTINGS
 *====================*/
/* Monochrome display (1-bit) */
#define LV_COLOR_DEPTH 1

/*=========================
   MEMORY SETTINGS
 *=========================*/
/* ESP32 has plenty of RAM */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)  /* 64KB for LVGL heap */

/*====================
   HAL SETTINGS
 *====================*/
/* Default display buffer size */
#define LV_DPI_DEF 130

/* Tick configuration */
#define LV_TICK_CUSTOM 1
#if LV_TICK_CUSTOM
  #define LV_TICK_CUSTOM_INCLUDE "Arduino.h"
  #define LV_TICK_CUSTOM_SYS_TIME_EXPR (millis())
#endif

/*================
 * LOG SETTINGS
 *================*/
#define LV_USE_LOG 1
#if LV_USE_LOG
  #define LV_LOG_LEVEL LV_LOG_LEVEL_WARN
  #define LV_LOG_PRINTF 1
#endif

/*================
 * FONT USAGE
 *================*/
/* Montserrat fonts with ASCII range */
#define LV_FONT_MONTSERRAT_8  0
#define LV_FONT_MONTSERRAT_10 0
#define LV_FONT_MONTSERRAT_12 0
#define LV_FONT_MONTSERRAT_14 1
#define LV_FONT_MONTSERRAT_16 0
#define LV_FONT_MONTSERRAT_18 0
#define LV_FONT_MONTSERRAT_20 1
#define LV_FONT_MONTSERRAT_22 0
#define LV_FONT_MONTSERRAT_24 1
#define LV_FONT_MONTSERRAT_26 0
#define LV_FONT_MONTSERRAT_28 0
#define LV_FONT_MONTSERRAT_30 0
#define LV_FONT_MONTSERRAT_32 1
#define LV_FONT_MONTSERRAT_34 0
#define LV_FONT_MONTSERRAT_36 0
#define LV_FONT_MONTSERRAT_38 0
#define LV_FONT_MONTSERRAT_40 0
#define LV_FONT_MONTSERRAT_42 0
#define LV_FONT_MONTSERRAT_44 0
#define LV_FONT_MONTSERRAT_46 0
#define LV_FONT_MONTSERRAT_48 0

/* Pixel perfect monospace font */
#define LV_FONT_UNSCII_8 0

/* Default font */
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/*===================
 * WIDGET USAGE
 *==================*/
/* Basic widgets */
#define LV_USE_LABEL    1
#define LV_USE_BUTTON   1
#define LV_USE_IMAGE    1
#define LV_USE_LINE     1
#define LV_USE_ARC      1

/* Extra widgets */
#define LV_USE_BAR      1
#define LV_USE_CHECKBOX 0
#define LV_USE_DROPDOWN 0
#define LV_USE_SLIDER   0
#define LV_USE_ROLLER   0
#define LV_USE_TEXTAREA 0
#define LV_USE_TABLE    0
#define LV_USE_CALENDAR 0
#define LV_USE_CHART    0
#define LV_USE_COLORWHEEL 0
#define LV_USE_IMGBTN   0
#define LV_USE_KEYBOARD 0
#define LV_USE_LED      0
#define LV_USE_LIST     0
#define LV_USE_MENU     0
#define LV_USE_MSGBOX   0
#define LV_USE_SPAN     0
#define LV_USE_SPINBOX  0
#define LV_USE_SPINNER  0
#define LV_USE_SWITCH   0
#define LV_USE_TABVIEW  0
#define LV_USE_TILEVIEW 0
#define LV_USE_WIN      0

/*==================
 * THEMES
 *==================*/
#define LV_USE_THEME_DEFAULT 1
#if LV_USE_THEME_DEFAULT
  #define LV_THEME_DEFAULT_DARK 0
  #define LV_THEME_DEFAULT_GROW 1
  #define LV_THEME_DEFAULT_TRANSITION_TIME 80
#endif

/*==================
 * LAYOUTS
 *==================*/
#define LV_USE_FLEX 1
#define LV_USE_GRID 1

/*====================
 * OTHERS
 *====================*/
/* Disable animations for e-ink */
#define LV_USE_SNAPSHOT 0
#define LV_USE_MONKEY   0
#define LV_USE_GRIDNAV  0
#define LV_USE_FRAGMENT 0

/* File system */
#define LV_USE_FS_STDIO 0
#define LV_USE_FS_POSIX 0
#define LV_USE_FS_WIN32 0
#define LV_USE_FS_FATFS 0
#define LV_USE_FS_LITTLEFS 0

/* PNG decoder */
#define LV_USE_PNG 0
#define LV_USE_BMP 0
#define LV_USE_SJPG 0
#define LV_USE_GIF 0
#define LV_USE_QRCODE 0

/* FreeType library */
#define LV_USE_FREETYPE 0

/* FFmpeg library */
#define LV_USE_FFMPEG 0

/* Others */
#define LV_USE_PERF_MONITOR 0
#define LV_USE_MEM_MONITOR 0

/*==================
 * EXAMPLES
 *==================*/
#define LV_BUILD_EXAMPLES 0

/*==================
 * DEMOS
 *==================*/
#define LV_USE_DEMO_WIDGETS 0
#define LV_USE_DEMO_BENCHMARK 0
#define LV_USE_DEMO_STRESS 0
#define LV_USE_DEMO_MUSIC 0

#endif /*LV_CONF_H*/

#endif /*End of "Content enable"*/
