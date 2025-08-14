#pragma once
#include "global.h"
#include "config_manager.h"
#include "logger.h"
#include <WiFi.h>

class WifiMgr {
public:
    bool begin(const WifiCfg& cfg, Logger& log);
    void tick(uint32_t now_ms);
    bool connected() const { return WiFi.status() == WL_CONNECTED; }
    IPAddress ip() const { return WiFi.localIP(); }
    void shutdown();
    bool finished() const { return _state == DONE; }
private:
    enum State { IDLE, SCANNING, CONNECTING, DONE } _state{IDLE};
    Logger* _log{nullptr};
    WifiCfg _cfg;
    uint32_t _connStart{0};
    size_t _idx{0};
    int _attempt{0};
};

extern WifiMgr wifiMgr;

