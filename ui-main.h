#pragma once
/**
 * @file ui-main.h
 * @brief LVGL based user interface manager.
 */

#include "global.h"
#include "logger.h"
#include "gps_mgr.h"
#include "wifi_mgr.h"

namespace UI {
  void begin(Logger& log, const Config& cfg);
  void setTimezone(int minutes);
  void showBoot();
  void showMainMenu();
  void showSatellite(const GpsStatus& st);
  void tick(uint32_t now, const GpsStatus& gps, const WifiMgr& wifi);
}

