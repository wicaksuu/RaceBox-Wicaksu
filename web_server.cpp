#include "web_server.h"
#include "gps_mgr.h"
#include "wifi_mgr.h"
#include <ArduinoJson.h>

WebSrv webServer;

void WebSrv::begin(GpsMgr* gps, WifiMgr* wifi) {
    _gps = gps;
    _wifi = wifi;
}

void WebSrv::start(Logger& log) {
    if (_running) return;
    _server.on("/", [](){ WebServer& s = webServer._server; s.send(200, "text/plain", "OK"); });
    _server.on("/status", [this](){
        StaticJsonDocument<256> doc;
        if (_wifi) {
            doc["wifi"]["connected"] = _wifi->connected();
            doc["wifi"]["ip"] = _wifi->ip().toString();
        }
        if (_gps) {
            GpsStatus st = _gps->status();
            doc["gps"]["fix"] = st.hasFix;
            doc["gps"]["sats"] = st.sats;
            doc["gps"]["hdop"] = st.hdop;
        }
        String out; serializeJson(doc, out);
        _server.send(200, "application/json", out);
    });
    _server.begin();
    log.info("WebServer started");
    _running = true;
}

void WebSrv::stop() {
    if (!_running) return;
    _server.stop();
    _running = false;
}

void WebSrv::tick() {
    if (_running) _server.handleClient();
}

