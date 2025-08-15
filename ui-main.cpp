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
static lv_obj_t* logCont = nullptr;
static lv_obj_t* satLabel = nullptr; //!< label for satellite info screen
static Logger* s_log = nullptr;
static int s_tz = 0;

static void logHook(const String& line) {
  if (!logCont) return;
  lv_color_t color = lv_color_white();
  String msg = line;
  if (line.startsWith("[INFO]")) { // success messages in green
    color = lv_palette_main(LV_PALETTE_GREEN);
    msg = line.substring(7);
  } else if (line.startsWith("[WARN]")) { // warnings in yellow
    color = lv_palette_main(LV_PALETTE_YELLOW);
    msg = line.substring(7);
  } else if (line.startsWith("[ERR]")) { // errors in red
    color = lv_palette_main(LV_PALETTE_RED);
    msg = line.substring(6);
  }
  lv_obj_t* lbl = lv_label_create(logCont); // create a new line label
  lv_label_set_long_mode(lbl, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(lbl, lv_pct(100));
  lv_obj_set_style_text_color(lbl, color, LV_PART_MAIN);
  String out = String("> ") + msg; // bullet style
  lv_label_set_text(lbl, out.c_str());
  lv_obj_scroll_to_view(lbl, LV_ANIM_OFF); // keep latest line visible
  if (lv_obj_get_child_cnt(logCont) > 100) { // limit to 100 lines
    lv_obj_t* first = lv_obj_get_child(logCont, 0);
    lv_obj_del(first);
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
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_180);
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
  // Set boot title in yellow and center it
  lv_label_set_text(topLabel, "WICAKSU RACE STATISTIC TOOLS");
  lv_obj_set_style_text_color(topLabel, lv_palette_main(LV_PALETTE_YELLOW), LV_PART_MAIN);
  lv_obj_align(topLabel, LV_ALIGN_TOP_MID, 0, 0);

  // Create scrolling container for boot log lines
  logCont = lv_obj_create(scr);
  lv_obj_set_size(logCont, SCREEN_WIDTH, SCREEN_HEIGHT - 20);
  lv_obj_align(logCont, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_obj_set_style_bg_color(logCont, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_border_width(logCont, 0, LV_PART_MAIN);
  lv_obj_set_scroll_dir(logCont, LV_DIR_VER);
  lv_obj_set_flex_flow(logCont, LV_FLEX_FLOW_COLUMN);

  if (s_log) s_log->onLine = logHook; // hook logger to append lines
}

void UI::showMainMenu() {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  // Restore top bar styling for runtime status
  lv_obj_set_style_text_color(topLabel, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(topLabel, LV_ALIGN_TOP_LEFT, 0, 0);

  // Create vertically stacked RACE and SATELIT buttons
  lv_obj_t* btnRace = lv_button_create(scr);
  lv_obj_set_size(btnRace, 200, 60);
  lv_obj_align(btnRace, LV_ALIGN_CENTER, 0, -40);
  lv_obj_set_style_bg_color(btnRace, lv_palette_main(LV_PALETTE_RED), LV_PART_MAIN);
  lv_obj_set_style_radius(btnRace, 30, LV_PART_MAIN);
  lv_obj_t* lblR = lv_label_create(btnRace);
  lv_label_set_text(lblR, "RACE");

  lv_obj_t* btnSat = lv_button_create(scr);
  lv_obj_set_size(btnSat, 200, 60);
  lv_obj_align(btnSat, LV_ALIGN_CENTER, 0, 40);
  lv_obj_set_style_bg_color(btnSat, lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);
  lv_obj_set_style_radius(btnSat, 30, LV_PART_MAIN);
  lv_obj_t* lblS = lv_label_create(btnSat);
  lv_label_set_text(lblS, "SATELIT");

  logCont = nullptr; // disable boot log
  satLabel = nullptr;
  if (s_log) s_log->onLine = nullptr; // detach logger from UI
}

void UI::showSatellite(const GpsStatus& st) {
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_text_color(topLabel, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(topLabel, LV_ALIGN_TOP_LEFT, 0, 0);
  satLabel = lv_label_create(scr);
  lv_label_set_text_fmt(satLabel, "Sats: %d", st.sats);
  lv_obj_set_style_text_color(satLabel, lv_color_white(), LV_PART_MAIN);
  lv_obj_align(satLabel, LV_ALIGN_CENTER, 0, 0);
  logCont = nullptr;
}

void UI::tick(uint32_t now, const GpsStatus& gps, const WifiMgr& wifi) {
  static uint32_t lastTick = 0;
  if (now - lastTick >= LVGL_TICK_MS) {
    lv_tick_inc(now - lastTick);
    lastTick = now;
  }
  lv_timer_handler();

  const char* ssid = wifi.connected() ? wifi.ssid().c_str() : "Wi-Fi OFF";
  char ip[32] = "-";
  if (wifi.connected()) {
    snprintf(ip, sizeof(ip), "%s", wifi.ip().toString().c_str());
  }
  char timebuf[24] = "-- --:--:--";
  if (gps.utc) {
    time_t local = gps.utc + s_tz * 60;
    struct tm* tm = gmtime(&local);
    static const char* days[] = {"SUN","MON","TUE","WED","THU","FRI","SAT"};
    snprintf(timebuf, sizeof(timebuf), "%s %02d:%02d:%02d", days[tm->tm_wday], tm->tm_hour, tm->tm_min, tm->tm_sec);
  }
  char buf[160];
  snprintf(buf, sizeof(buf), "%s  %s  ACC:%.2f SAT:%d ALT:%.2f  %s  GPS:%.2f Hz",
           timebuf, ssid, gps.acc_m, gps.sats, gps.alt_m, ip, gps.rate_hz);
  if (!logCont && topLabel) { // avoid overwriting boot title
    lv_label_set_text(topLabel, buf);
  }
  if (satLabel) {
    lv_label_set_text_fmt(satLabel, "Sats: %d", gps.sats);
  }
}


void UI::setTimezone(int minutes) {
  s_tz = minutes;
}

