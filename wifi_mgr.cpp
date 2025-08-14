#include "wifi_mgr.h"

bool WifiMgr::begin(const WifiCfg& cfg, Logger& log) {
  _cfg = cfg;
  _log = &log;
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  _ssid = ""; // clear previous SSID
  _log->info("WiFi scanning...");
  WiFi.scanNetworks(true); // async
  _state = SCANNING;
  _last = millis();
  return true;
}

void WifiMgr::shutdown() {
  WebSrv::stop();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  _ssid = ""; // mark as disconnected
  _state = DONE;
  if (_log) _log->warn("WiFi OFF");
}

void WifiMgr::tick(uint32_t now) {
  switch (_state) {
    case SCANNING: {
      int n = WiFi.scanComplete();
      if (n >= 0) {
        _log->info("WiFi scan complete: %d", n);
        // match known networks
        _candidates.clear();
        for (int i=0;i<n;i++) {
          String ssid = WiFi.SSID(i);
          for (const auto& k : _cfg.known) {
            if (ssid == k.ssid) {
              _candidates.push_back(k);
              _log->info("WiFi found: %s", ssid.c_str()); // log each matched SSID
            }
          }
        }
        std::sort(_candidates.begin(), _candidates.end(),[](const WifiKnown& a,const WifiKnown& b){return a.priority>b.priority;});
        _current = -1;
        _state = CONNECTING;
      }
      break;
    }
    case CONNECTING: {
      if (connected()) {
        _ssid = WiFi.SSID();
        _log->info("WiFi connected: %s", _ssid.c_str());
        WebSrv::start(*_log);
        _state = DONE;
        break;
      }
      if (_current < 0) {
        if (_candidates.empty()) { shutdown(); break; }
        _current = 0; _attempts = 0;
        _log->info("Connecting to %s", _candidates[_current].ssid.c_str());
        WiFi.begin(_candidates[_current].ssid.c_str(), _candidates[_current].pass.c_str());
        _last = now;
      } else {
        if (now - _last > WIFI_CONNECT_TIMEOUT_MS) {
          _attempts++;
          if (_attempts >= _cfg.connect_attempts) {
            _current++; _attempts=0;
            if (_current >= (int)_candidates.size()) { shutdown(); break; }
            _log->warn("Retry next SSID %s", _candidates[_current].ssid.c_str());
            WiFi.begin(_candidates[_current].ssid.c_str(), _candidates[_current].pass.c_str());
            _last = now;
          } else {
            _log->warn("Retry %s (%d)", _candidates[_current].ssid.c_str(), _attempts);
            WiFi.disconnect();
            WiFi.begin(_candidates[_current].ssid.c_str(), _candidates[_current].pass.c_str());
            _last = now;
          }
        }
      }
      break;
    }
    default:
      break;
  }
}

