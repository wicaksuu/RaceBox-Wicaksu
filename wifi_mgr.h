#pragma once
/**
 * @file wifi_mgr.h
 * @brief Wi-Fi state machine for scanning and connecting.
 */

#include "global.h"
#include "logger.h"
#include "web_server.h"

class WifiMgr {
public:
  bool begin(const WifiCfg& cfg, Logger& log);
  void tick(uint32_t now);
  bool connected() const { return WiFi.status() == WL_CONNECTED; }
  IPAddress ip() const { return WiFi.localIP(); }
  void shutdown();
  bool done() const { return _state == DONE; }
private:
  enum State { IDLE, SCANNING, CONNECTING, DONE } _state = IDLE;
  WifiCfg _cfg;
  Logger* _log = nullptr;
  uint32_t _last = 0;
  std::vector<WifiKnown> _candidates;
  int _current = -1;
  int _attempts = 0;
};

