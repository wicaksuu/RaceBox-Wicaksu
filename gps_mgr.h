#pragma once
#include "global.h"
#include "logger.h"
#include "config_manager.h"
#include <time.h>
#include <HardwareSerial.h>
#include <vector>

/** Runtime GPS status */
struct GpsStatus {
    bool hasFix{false};
    float hdop{0};
    float acc_m{0};
    int sats{0};
    float rate_hz{0};
    bool timeValid{false};
    tm time{}; // UTC
};

/**
 * @brief u-blox M10 GPS manager.
 */
class GpsMgr {
public:
    bool begin(const GpsCfg& cfg, Logger& log);
    void tick(uint32_t now_ms);
    GpsStatus status() const { return _stat; }

private:
    void sendRate(uint8_t hz);
    void parseLine(const String& line, uint32_t now_ms);
    HardwareSerial* _ser{nullptr};
    GpsCfg _cfg;
    Logger* _log{nullptr};
    String _lineBuf;
    uint32_t _lastSentence{0};
    GpsStatus _stat;
};

extern GpsMgr gpsMgr;

