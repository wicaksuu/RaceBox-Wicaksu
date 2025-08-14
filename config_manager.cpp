#include "config_manager.h"
#include "logger.h"

ConfigManager configManager;

bool ConfigManager::mountSD() {
    if (SD.begin(PIN_SD_CS)) {
        return true;
    }
    return false;
}

static void writeDefault(File& f, const Config& cfg) {
    StaticJsonDocument<512> doc;
    doc["time"]["tz_offset_minutes"] = cfg.time.tz_offset_minutes;
    JsonObject gps = doc.createNestedObject("gps");
    gps["uart_rx"] = cfg.gps.uart_rx;
    gps["uart_tx"] = cfg.gps.uart_tx;
    gps["baud"]    = cfg.gps.baud;
    gps["rate_hz"] = cfg.gps.rate_hz;
    JsonObject wifi = doc.createNestedObject("wifi");
    wifi["enable"] = cfg.wifi.enable;
    wifi["connect_attempts"] = cfg.wifi.connect_attempts;
    JsonArray arr = wifi.createNestedArray("known");
    for (auto& k : cfg.wifi.known) {
        JsonObject o = arr.createNestedObject();
        o["ssid"] = k.ssid;
        o["pass"] = k.pass;
        o["priority"] = k.priority;
    }
    serializeJsonPretty(doc, f);
}

bool ConfigManager::load(Config& out) {
    if (!SD.exists(CONFIG_DIR)) {
        SD.mkdir(CONFIG_DIR);
    }
    if (!SD.exists(CONFIG_FILE)) {
        // create default config
        Config def;
        def.wifi.known.push_back({"Wicaksu", "wicaksuu", 1});
        def.wifi.known.push_back({"HP_TETHER", "yyyyy", 5});
        File f = SD.open(CONFIG_FILE, FILE_WRITE);
        if (!f) return false;
        writeDefault(f, def);
        f.close();
        out = def;
        return true;
    }
    File f = SD.open(CONFIG_FILE, FILE_READ);
    if (!f) return false;
    StaticJsonDocument<1024> doc;
    DeserializationError err = deserializeJson(doc, f);
    f.close();
    if (err) return false;
    out.time.tz_offset_minutes = doc["time"]["tz_offset_minutes"].as<int>();
    out.gps.uart_rx = doc["gps"]["uart_rx"].as<int>();
    out.gps.uart_tx = doc["gps"]["uart_tx"].as<int>();
    out.gps.baud    = doc["gps"]["baud"].as<uint32_t>();
    out.gps.rate_hz = doc["gps"]["rate_hz"].as<uint8_t>();
    out.wifi.enable = doc["wifi"]["enable"].as<bool>();
    out.wifi.connect_attempts = doc["wifi"]["connect_attempts"].as<int>();
    out.wifi.known.clear();
    for (JsonObject o : doc["wifi"]["known"].as<JsonArray>()) {
        WifiKnown k; k.ssid = o["ssid"].as<const char*>(); k.pass = o["pass"].as<const char*>(); k.priority = o["priority"].as<int>();
        out.wifi.known.push_back(k);
    }
    return true;
}

bool ConfigManager::save(const Config& in) {
    File f = SD.open(CONFIG_FILE, FILE_WRITE);
    if (!f) return false;
    writeDefault(f, in);
    f.close();
    return true;
}

