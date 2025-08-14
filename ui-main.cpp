#include "ui-main.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

#define SCREEN_WIDTH  320
#define SCREEN_HEIGHT 240
#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
static uint32_t draw_buf[DRAW_BUF_SIZE/4];

static TFT_eSPI tft;
static SPIClass touchSPI(VSPI);
static XPT2046_Touchscreen ts(PIN_TS_CS, PIN_TS_IRQ);
static lv_display_t* disp;
static lv_indev_t* indev;
static lv_obj_t* topLabel = nullptr;
static lv_obj_t* logArea = nullptr;
static lv_obj_t* satLabel = nullptr;
static Logger* s_log = nullptr;
static int s_tz = 0;

static void logHook(const String& line) {
  if (logArea) {
    lv_textarea_add_text(logArea, (line + "\n").c_str());
    lv_textarea_scroll_to_bottom(logArea);
  }
}

static void touchscreen_read(lv_indev_t * indev, lv_indev_data_t * data) {
  if(ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    int x = map(p.x, 200, 3700, 1, SCREEN_WIDTH);
    int y = map(p.y, 240, 3800, 1, SCREEN_HEIGHT);
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = x;
    data->point.y = y;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void UI::begin(Logger& log, const Config& cfg) {
  s_log = &log;
  s_tz = cfg.time.tz_offset_minutes;
  lv_init();
  touchSPI.begin(PIN_TS_CLK, PIN_TS_MISO, PIN_TS_MOSI, PIN_TS_CS);
  ts.begin(touchSPI);
  ts.setRotation(2);
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
  indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, touchscreen_read);
  lv_obj_set_style_bg_color(lv_screen_active(), lv_color_black(), LV_PART_MAIN); // dark background
  topLabel = lv_label_create(lv_layer_top());
  lv_obj_set_width(topLabel, SCREEN_WIDTH); // keep single line without overlap
  lv_label_set_long_mode(topLabel, LV_LABEL_LONG_CLIP);
  lv_obj_set_style_text_color(topLabel, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(topLabel, LV_ALIGN_TOP_LEFT, 0, 0);
}

void UI::showBoot() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN); // keep dark theme
  logArea = lv_textarea_create(scr);
  lv_obj_set_size(logArea, SCREEN_WIDTH, SCREEN_HEIGHT-20);
  lv_obj_align(logArea, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_text_color(logArea, lv_color_white(), LV_PART_MAIN);
  if (s_log) s_log->onLine = logHook;
}

void UI::showMainMenu() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  lv_obj_t* btnRace = lv_button_create(scr);
  lv_obj_set_size(btnRace, 120, 80);
  lv_obj_align(btnRace, LV_ALIGN_CENTER, -80, 0);
  lv_obj_t* lblR = lv_label_create(btnRace); // label for race button
  lv_label_set_text(lblR, "RACE");
  lv_obj_t* btnSat = lv_button_create(scr);
  lv_obj_set_size(btnSat, 120, 80);
  lv_obj_align(btnSat, LV_ALIGN_CENTER, 80, 0);
  lv_obj_t* lblS = lv_label_create(btnSat); // label for satellite button
  lv_label_set_text(lblS, "SATELLITE");
  logArea = nullptr;
  satLabel = nullptr;
}

void UI::showSatellite(const GpsStatus& st) {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  satLabel = lv_label_create(scr);
  lv_label_set_text_fmt(satLabel, "Sats: %d", st.sats);
  lv_obj_set_style_text_color(satLabel, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(satLabel, LV_ALIGN_CENTER, 0, 0);
  logArea = nullptr;
}

void UI::tick(uint32_t now, const GpsStatus& gps, const WifiMgr& wifi) {
  static uint32_t lastTick = 0;
  if (now - lastTick >= LVGL_TICK_MS) {
    lv_tick_inc(now - lastTick);
    lastTick = now;
  }
  lv_timer_handler();

  char ip[32];
  if (wifi.connected()) {
    snprintf(ip, sizeof(ip), "%s", wifi.ip().toString().c_str());
  } else {
    snprintf(ip, sizeof(ip), "Wi-Fi OFF");
  }
  char timebuf[16] = "--:--";
  if (gps.utc) {
    time_t local = gps.utc + s_tz * 60;
    struct tm* tm = gmtime(&local);
    snprintf(timebuf, sizeof(timebuf), "%02d:%02d", tm->tm_hour, tm->tm_min);
  }
  char buf[96];
  snprintf(buf, sizeof(buf), "%s | %s | HDOP %.1f SAT %d Hz %.1f", ip, timebuf, gps.hdop, gps.sats, gps.rate_hz);
  if (topLabel) lv_label_set_text(topLabel, buf); // update top status line
  if (satLabel) {
    lv_label_set_text_fmt(satLabel, "Sats: %d", gps.sats);
  }
}


void UI::setTimezone(int minutes) {
  s_tz = minutes;
}

