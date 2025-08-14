#include "ui-main.h"

UI ui;
UI* UI::_instance = nullptr;

// Map raw touch coordinates to the rotated LVGL screen.
void UI::touch_cb(lv_indev_t*, lv_indev_data_t* data) {
    if (!_instance) return;
    if (_instance->_ts.tirqTouched() && _instance->_ts.touched()) {
        TS_Point p = _instance->_ts.getPoint();
        // Raw calibration values from sample. Convert to 240x320 then rotate to 320x240.
        int xr = map(p.x, 200, 3700, 0, 240);
        int yr = map(p.y, 240, 3800, 0, 320);
        // Rotate 270 degrees and clamp to valid range to avoid LVGL warnings.
        int x = constrain(yr, 0, 319);
        int y = constrain(240 - xr, 0, 239);
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
    _instance = this;
    lv_init();
    // Enable dark theme with red accents for a racing look.
    lv_theme_t* th = lv_theme_default_init(nullptr,
                                           lv_palette_main(LV_PALETTE_GREY),
                                           lv_palette_main(LV_PALETTE_RED),
                                           LV_THEME_DEFAULT_DARK,
                                           LV_FONT_DEFAULT);
    lv_theme_set_act(th);

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
    lv_obj_set_size(_topBar, 320, 20);
    lv_obj_align(_topBar, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(_topBar, lv_color_hex(0x101010), 0);
    lv_obj_clear_flag(_topBar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_move_foreground(_topBar); // keep bar fixed on top
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
    lv_obj_clear_flag(_scrBoot, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(_scrBoot, lv_color_hex(0x000000), 0);
    createTopBar();
    _logLabel = lv_label_create(_scrBoot);
    lv_obj_set_width(_logLabel, 320);
    lv_obj_align(_logLabel, LV_ALIGN_TOP_LEFT, 0, 22);
    lv_label_set_long_mode(_logLabel, LV_LABEL_LONG_CLIP);
    lv_label_set_text(_logLabel, "");
    logger.onLine = [this](const String& line){ appendLog(line); };
}

void UI::appendLog(const String& line) {
    String all;
    size_t count = logger.lines().size();
    size_t start = count > 10 ? count - 10 : 0; // show last 10 lines to avoid scroll
    for (size_t i = start; i < count; ++i) {
        all += logger.lines()[i] + "\n";
    }
    if (_logLabel) lv_label_set_text(_logLabel, all.c_str());
}

void UI::showMainMenu() {
    _scrMenu = lv_obj_create(NULL);
    lv_screen_load(_scrMenu);
    lv_obj_clear_flag(_scrMenu, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(_scrMenu, lv_color_hex(0x000000), 0);
    createTopBar();
    // Red buttons on dark background for racing look.
    lv_obj_t* btnRace = lv_button_create(_scrMenu);
    lv_obj_set_size(btnRace, 120, 80);
    lv_obj_align(btnRace, LV_ALIGN_CENTER, -80, 20);
    lv_obj_set_style_bg_color(btnRace, lv_color_hex(0x400000), 0);
    lv_obj_set_style_border_color(btnRace, lv_color_hex(0xFF0000), 0);
    lv_obj_set_style_border_width(btnRace, 2, 0);
    lv_obj_t* lblR = lv_label_create(btnRace);
    lv_label_set_text(lblR, "RACE");
    lv_obj_center(lblR);

    lv_obj_t* btnSat = lv_button_create(_scrMenu);
    lv_obj_set_size(btnSat, 120, 80);
    lv_obj_align(btnSat, LV_ALIGN_CENTER, 80, 20);
    lv_obj_set_style_bg_color(btnSat, lv_color_hex(0x002040), 0);
    lv_obj_set_style_border_color(btnSat, lv_color_hex(0x00BFFF), 0);
    lv_obj_set_style_border_width(btnSat, 2, 0);
    lv_obj_t* lblS = lv_label_create(btnSat);
    lv_label_set_text(lblS, "SATELIT");
    lv_obj_center(lblS);
}

void UI::showSatelit(const GpsStatus& st) {
    if (!_scrSat) {
        _scrSat = lv_obj_create(NULL);
        lv_obj_clear_flag(_scrSat, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_style_bg_color(_scrSat, lv_color_hex(0x000000), 0);
        lv_obj_t* lbl = lv_label_create(_scrSat);
        lv_obj_set_width(lbl, 320);
        lv_obj_align(lbl, LV_ALIGN_TOP_LEFT, 0, 22);
        lv_label_set_long_mode(lbl, LV_LABEL_LONG_CLIP);
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

