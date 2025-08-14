#include "wifi_mgr.h"
#include "web_server.h"
#include <algorithm>

WifiMgr wifiMgr;

bool WifiMgr::begin(const WifiCfg& cfg, Logger& log) {
    _cfg = cfg;
    _log = &log;
    _state = SCANNING;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true);
    WiFi.scanNetworks(true); // async scan
    _log->info("WiFi scanning...");
    // sort known by priority (lower number = higher priority)
    std::sort(_cfg.known.begin(), _cfg.known.end(), [](const WifiKnown& a, const WifiKnown& b){ return a.priority < b.priority; });
    return true;
}

void WifiMgr::tick(uint32_t now_ms) {
    if (_state == SCANNING) {
        int n = WiFi.scanComplete();
        if (n >= 0) {
            _log->info("Scan done: %d networks", n);
            _state = CONNECTING;
            _connStart = 0;
            _idx = 0;
        }
    } else if (_state == CONNECTING) {
        if (connected()) {
            _log->info("WiFi connected: %s", WiFi.localIP().toString().c_str());
            webServer.start(*_log);
            _state = DONE;
        } else {
            if (_connStart == 0 && _idx < _cfg.known.size()) {
                WifiKnown& k = _cfg.known[_idx];
                _log->info("Connecting to %s", k.ssid.c_str());
                WiFi.begin(k.ssid.c_str(), k.pass.c_str());
                _connStart = now_ms;
                _attempt++;
            } else if (_connStart != 0 && now_ms - _connStart > WIFI_CONNECT_TIMEOUT_MS) {
                _log->warn("WiFi timeout");
                _idx++;
                _connStart = 0;
                if (_attempt >= _cfg.connect_attempts || _idx >= _cfg.known.size()) {
                    _log->error("WiFi failed");
                    shutdown();
                    _state = DONE;
                }
            }
        }
    } else if (_state == DONE) {
        if (!connected()) {
            webServer.stop();
        }
    }
}

void WifiMgr::shutdown() {
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
}

