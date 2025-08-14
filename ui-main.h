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
    /** @brief Initialize LVGL, display, touch, and dark theme. */
    void begin(Logger& log, const Config& cfg);
    /** @brief Show boot screen that receives log lines. */
    void showBoot();
    /** @brief Display the main menu with racing look. */
    void showMainMenu();
    /** @brief Show satellite status screen. */
    void showSatelit(const GpsStatus& st);
    /** @brief Update top bar and process LVGL tasks. */
    void tick(uint32_t now_ms, const GpsStatus& gps, const WifiMgr& wifi);
private:
    /** @brief Read touchscreen and feed coordinates to LVGL. */
    static void touch_cb(lv_indev_t*, lv_indev_data_t*);
    static UI* _instance;
    /** @brief Create a fixed top bar for the active screen. */
    void createTopBar();
    /** @brief Append a new line to boot log label without scrolling. */
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

