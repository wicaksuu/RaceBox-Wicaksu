#pragma once
#include "global.h"
#include "logger.h"
#include <WebServer.h>

class GpsMgr; // forward
class WifiMgr; // forward

/**
 * @brief Simple WebServer wrapper.
 */
class WebSrv {
public:
    void begin(GpsMgr* gps, WifiMgr* wifi);
    void start(Logger& log);
    void stop();
    void tick();
    bool running() const { return _running; }
private:
    WebServer _server{80};
    bool _running{false};
    GpsMgr* _gps{nullptr};
    WifiMgr* _wifi{nullptr};
};

extern WebSrv webServer;

