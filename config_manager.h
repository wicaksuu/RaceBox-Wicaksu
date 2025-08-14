#pragma once
#include "global.h"
#include <vector>
#include <ArduinoJson.h>
#include <SD.h>

/** Wi-Fi known network entry */
struct WifiKnown { String ssid; String pass; int priority; };

struct TimeCfg { int tz_offset_minutes{420}; };
struct GpsCfg { int uart_rx{GPS_RX}; int uart_tx{GPS_TX}; uint32_t baud{115200}; uint8_t rate_hz{20}; };
struct WifiCfg {
    bool enable{true};
    std::vector<WifiKnown> known;
    int connect_attempts{10};
};
struct Config {
    TimeCfg time;
    GpsCfg gps;
    WifiCfg wifi;
};

/**
 * @brief Manage configuration stored on microSD as JSON.
 */
class ConfigManager {
public:
    bool mountSD();
    bool load(Config& out);
    bool save(const Config& in);
};

extern ConfigManager configManager;

