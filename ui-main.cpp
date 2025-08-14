#include "ui-main.h"

UI ui;
static UI* s_ui = nullptr;

static void touch_cb(lv_indev_t*, lv_indev_data_t* data) {
    if (!s_ui) return;
    if (s_ui->_ts.tirqTouched() && s_ui->_ts.touched()) {
        TS_Point p = s_ui->_ts.getPoint();
        int x = map(p.x, 200, 3700, 1, 240);
        int y = map(p.y, 240, 3800, 1, 320);
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

void UI::begin(Logger& log, const Config& cfg) {
    _log = &log;
    _tzMin = cfg.time.tz_offset_minutes;
    s_ui = this;
    lv_init();
    _tsSPI.begin(PIN_TS_CLK, PIN_TS_MISO, PIN_TS_MOSI, PIN_TS_CS);
    _ts.begin(_tsSPI);
    _ts.setRotation(2);
    static uint32_t draw_buf[240 * 320 / 10];
    lv_display_t* disp = lv_tft_espi_create(240, 320, draw_buf, sizeof(draw_buf));
    lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270);
    lv_indev_t* indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(indev, touch_cb);
    createTopBar();
}

void UI::createTopBar() {
    _topBar = lv_obj_create(lv_screen_active());
    lv_obj_set_size(_topBar, 240, 20);
    lv_obj_align(_topBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(_topBar, lv_color_hex(0x202020), 0);
    _labelIP = lv_label_create(_topBar);
    lv_obj_align(_labelIP, LV_ALIGN_LEFT_MID, 2, 0);
    _labelTime = lv_label_create(_topBar);
    lv_obj_align(_labelTime, LV_ALIGN_CENTER, 0, 0);
    _labelGPS = lv_label_create(_topBar);
    lv_obj_align(_labelGPS, LV_ALIGN_RIGHT_MID, -2, 0);
}

void UI::showBoot() {
    _scrBoot = lv_obj_create(NULL);
    lv_screen_load(_scrBoot);
    createTopBar();
    _logLabel = lv_label_create(_scrBoot);
    lv_obj_align(_logLabel, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_label_set_text(_logLabel, "");
    logger.onLine = [this](const String& line){ appendLog(line); };
}

void UI::appendLog(const String& line) {
    String all;
    for (const auto& l : logger.lines()) {
        all += l + "\n";
    }
    if (_logLabel) lv_label_set_text(_logLabel, all.c_str());
}

void UI::showMainMenu() {
    _scrMenu = lv_obj_create(NULL);
    lv_screen_load(_scrMenu);
    createTopBar();
    lv_obj_t* btnRace = lv_button_create(_scrMenu);
    lv_obj_set_size(btnRace, 100, 60);
    lv_obj_align(btnRace, LV_ALIGN_CENTER, -60, 20);
    lv_label_set_text(lv_label_create(btnRace), "RACE");
    lv_obj_t* btnSat = lv_button_create(_scrMenu);
    lv_obj_set_size(btnSat, 100, 60);
    lv_obj_align(btnSat, LV_ALIGN_CENTER, 60, 20);
    lv_label_set_text(lv_label_create(btnSat), "SATELIT");
}

void UI::showSatelit(const GpsStatus& st) {
    if (!_scrSat) {
        _scrSat = lv_obj_create(NULL);
        lv_obj_t* lbl = lv_label_create(_scrSat);
        lv_obj_set_width(lbl, lv_pct(100));
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 22);
        _logLabel = lbl;
    }
    lv_screen_load(_scrSat);
    createTopBar();
    lv_label_set_text_fmt(_logLabel, "SAT:%d HDOP:%.1f", st.sats, st.hdop);
}

void UI::tick(uint32_t now_ms, const GpsStatus& gps, const WifiMgr& wifi) {
    if (wifi.connected()) {
        lv_label_set_text_fmt(_labelIP, "%s", wifi.ip().toString().c_str());
    } else {
        lv_label_set_text(_labelIP, "WiFi OFF");
    }
    if (gps.timeValid) {
        int hour = gps.time.tm_hour;
        int min = gps.time.tm_min;
        int sec = gps.time.tm_sec;
        hour += _tzMin / 60;
        if (hour >= 24) hour -= 24;
        lv_label_set_text_fmt(_labelTime, "%02d:%02d:%02d", hour, min, sec);
    } else {
        lv_label_set_text(_labelTime, "--:--");
    }
    lv_label_set_text_fmt(_labelGPS, "HDOP %.1f SAT %d %.1fHz", gps.hdop, gps.sats, gps.rate_hz);
    if (now_ms - _lastLvTick >= LVGL_TICK_MS) {
        lv_tick_inc(LVGL_TICK_MS);
        _lastLvTick = now_ms;
    }
    lv_timer_handler();
}

