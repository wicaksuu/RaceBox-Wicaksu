#pragma once
/* Global pins, constants, and shared objects for CYD (ESP32-2432S028R) + LVGL 9.x.
   - Keep this header lean: only declarations and inline constexpr.
   - Definitions live in global.cpp to avoid multiple definitions.
*/
#include <Arduino.h>
#include <SPI.h>
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// ---------- Display & Touch ----------
inline constexpr int SCREEN_WIDTH  = 240;  // physical panel size (portrait)
inline constexpr int SCREEN_HEIGHT = 320;
inline constexpr lv_display_rotation_t DISP_ROTATION = LV_DISPLAY_ROTATION_270; // landscape

// XPT2046 touch pins (on CYD)
inline constexpr int XPT2046_IRQ  = 36;  // T_IRQ (input-only)
inline constexpr int XPT2046_MOSI = 32;  // T_DIN
inline constexpr int XPT2046_MISO = 39;  // T_OUT (input-only)
inline constexpr int XPT2046_CLK  = 25;  // T_CLK
inline constexpr int XPT2046_CS   = 33;  // T_CS
inline constexpr uint8_t TOUCH_ROTATION = 2; // 0..3, tune if axes are flipped

// Raw touch calibration (tweak to your panel)
inline constexpr int TOUCH_X_MIN = 200;
inline constexpr int TOUCH_X_MAX = 3700;
inline constexpr int TOUCH_Y_MIN = 240;
inline constexpr int TOUCH_Y_MAX = 3800;

// ---------- LVGL buffers ----------
#ifndef LV_COLOR_DEPTH
#warning "LV_COLOR_DEPTH not defined, assuming 16-bit"
#define LV_COLOR_DEPTH 16
#endif
inline constexpr int    DRAW_BUF_DIV        = 10; // ~10% of screen
inline constexpr size_t DRAW_BUF_SIZE_BYTES =
  (size_t(SCREEN_WIDTH) * size_t(SCREEN_HEIGHT) / DRAW_BUF_DIV) * (LV_COLOR_DEPTH / 8);

// Extern storage defined in global.cpp
extern uint32_t LV_DRAW_BUF[DRAW_BUF_SIZE_BYTES / sizeof(uint32_t)];

// ---------- Shared drivers/handles ----------
extern SPIClass touchscreenSPI;                // VSPI for touch
extern XPT2046_Touchscreen touchscreen;       // Touch driver
extern lv_display_t* g_display;               // LVGL display handle
extern lv_indev_t*   g_touch_indev;           // LVGL input device

// ---------- Logging ----------
void lv_log_print_cb(lv_log_level_t level, const char * buf);

// ---------- Sanity checks ----------
static_assert(SCREEN_WIDTH > 0 && SCREEN_HEIGHT > 0, "Invalid screen size");
