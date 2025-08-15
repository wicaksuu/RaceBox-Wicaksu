#include "global.h"

// LVGL partial draw buffer (single buffer is fine for Arduino)
uint32_t LV_DRAW_BUF[DRAW_BUF_SIZE_BYTES / sizeof(uint32_t)] = {0};

// Touch SPI & driver (bind to VSPI; pins are set in .begin below)
SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);

// LVGL handles
lv_display_t* g_display = nullptr;
lv_indev_t*   g_touch_indev = nullptr;

// Simple LVGL log pipe to Serial
void lv_log_print_cb(lv_log_level_t level, const char * buf) {
  LV_UNUSED(level);
  Serial.println(buf);
  Serial.flush();
}
