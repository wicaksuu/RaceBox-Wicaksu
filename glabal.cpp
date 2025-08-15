#include "global.h"

// ===== Buffers (double) =====
uint32_t LV_DRAW_BUF_A[DRAW_BUF_SIZE_BYTES / sizeof(uint32_t)] = {0};
uint32_t LV_DRAW_BUF_B[DRAW_BUF_SIZE_BYTES / sizeof(uint32_t)] = {0};

// ===== Drivers/handles =====
TFT_eSPI tft; // uses TFT_eSPI User_Setup.h
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
SPIClass sdSPI = SPIClass(HSPI);
HardwareSerial GPSSerial(1);
WebServer server(80);

lv_display_t* g_display = nullptr;
lv_indev_t*   g_touch_indev = nullptr;

void lv_log_print_cb(lv_log_level_t level, const char* buf) {
  LV_UNUSED(level);
  // Di-forward ke logger kita di logview.cpp (didafarkan di init)
  // Biarkan kosong di sini; callback asli akan di-override.
}
