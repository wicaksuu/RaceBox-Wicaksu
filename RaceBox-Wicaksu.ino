#include "global.h"
#include "logger.h"
#include "config_manager.h"
#include "gps_mgr.h"
#include "wifi_mgr.h"
#include "web_server.h"
#include "ui-main.h"

Logger logger;
Config cfg;
GpsMgr gps;
WifiMgr wifi;

String statusProvider() {
  GpsStatus st = gps.status();
  DynamicJsonDocument doc(256);
  doc["wifi"]["connected"] = wifi.connected();
  if (wifi.connected()) doc["wifi"]["ip"] = wifi.ip().toString();
  doc["gps"]["fix"] = st.hasFix;
  doc["gps"]["sats"] = st.sats;
  doc["gps"]["hdop"] = st.hdop;
  doc["gps"]["alt"] = st.alt_m;
  doc["gps"]["rate"] = st.rate_hz;
  String out; serializeJson(doc, out); return out;
}

void setup() {
  Serial.begin(115200);
  logger.init(&Serial, 100);
  Config tmp; UI::begin(logger, tmp); UI::showBoot();

  if (ConfigManager::mountSD(&logger)) {
    ConfigManager::load(cfg, logger);
  }
  UI::setTimezone(cfg.time.tz_offset_minutes);

  if (gps.begin(cfg.gps, logger)) {
    logger.info("GPS begin success");
  } else {
    logger.error("GPS begin failed");
  }
  // Start Wi-Fi manager only when enabled in configuration
  if (cfg.wifi.enable) {
    wifi.begin(cfg.wifi, logger);
  } else {
    wifi.shutdown();
  }
  WebSrv::setStatusProvider(statusProvider);
}

void loop() {
  uint32_t now = millis();
  gps.tick(now);
  wifi.tick(now);
  WebSrv::tick();
  UI::tick(now, gps.status(), wifi);
  static bool menuShown = false;
  if (!menuShown && wifi.done() && now > 8000) {
    UI::showMainMenu();
    menuShown = true;
  }
}

