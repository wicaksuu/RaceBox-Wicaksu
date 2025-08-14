#pragma once
#include "global.h"
#include "logger.h"
#include "config_manager.h"
#include "gps_mgr.h"
#include "wifi_mgr.h"
#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

/**
 * @brief Main UI manager using LVGL.
 */
class UI {
public:
    void begin(Logger& log, const Config& cfg);
    void showBoot();
    void showMainMenu();
    void showSatelit(const GpsStatus& st);
    void tick(uint32_t now_ms, const GpsStatus& gps, const WifiMgr& wifi);
private:
    void createTopBar();
    void appendLog(const String& line);
    Logger* _log{nullptr};
    int _tzMin{420};
    lv_obj_t* _topBar{nullptr};
    lv_obj_t* _labelIP{nullptr};
    lv_obj_t* _labelTime{nullptr};
    lv_obj_t* _labelGPS{nullptr};
    lv_obj_t* _logLabel{nullptr};
    lv_obj_t* _scrBoot{nullptr};
    lv_obj_t* _scrMenu{nullptr};
    lv_obj_t* _scrSat{nullptr};
    TFT_eSPI _tft;
    SPIClass _tsSPI = SPIClass(VSPI);
    XPT2046_Touchscreen _ts = XPT2046_Touchscreen(PIN_TS_CS, PIN_TS_IRQ);
    uint32_t _lastLvTick{0};
};

extern UI ui;

