#include "global.h"
#include "logger.h"
#include "config_manager.h"
#include "gps_mgr.h"
#include "wifi_mgr.h"
#include "web_server.h"
#include "ui-main.h"

Config g_cfg;

void setup() {
  Serial.begin(115200);
  logger.init(&Serial, LOGGER_MAX_LINES);
  ui.begin(logger, g_cfg);
  ui.showBoot();
  if (configManager.mountSD()) {
    logger.info("SD mounted");
    if (configManager.load(g_cfg)) {
      logger.info("Config loaded");
    } else {
      logger.error("Config load failed");
    }
  } else {
    logger.error("SD mount failed");
  }
  webServer.begin(&gpsMgr, &wifiMgr);
  gpsMgr.begin(g_cfg.gps, logger);
  if (g_cfg.wifi.enable) wifiMgr.begin(g_cfg.wifi, logger); else wifiMgr.shutdown();
}

void loop() {
  uint32_t now = millis();
  gpsMgr.tick(now);
  wifiMgr.tick(now);
  webServer.tick();
  ui.tick(now, gpsMgr.status(), wifiMgr);
  static bool menuShown = false;
  if (!menuShown && wifiMgr.finished()) {
    ui.showMainMenu();
    menuShown = true;
  }
}

