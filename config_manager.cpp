#include "config_manager.h"

namespace {
const char* CONFIG_DIR = "/config";
const char* CONFIG_FILE = "/config/system.json";

Config makeDefault() {
  Config cfg;
  cfg.time.tz_offset_minutes = 420; // UTC+7
  cfg.gps.uart_rx = 27;
  cfg.gps.uart_tx = 22;
  cfg.gps.baud = 115200;
  cfg.gps.rate_hz = 20;
  cfg.gps.dynamic_model = "automotive";
  cfg.wifi.enable = true;
  cfg.wifi.connect_attempts = 10;
  cfg.wifi.known.push_back({"Wicaksu","wicaksuu",1});
  cfg.wifi.known.push_back({"HP_TETHER","yyyyy",5});
  cfg.ui.theme = "racing-dark";
  return cfg;
}

bool ensureDir() {
  if (!SD.exists(CONFIG_DIR)) {
    return SD.mkdir(CONFIG_DIR);
  }
  return true;
}
}

bool ConfigManager::mountSD(Logger* log) {
  if (SD.begin(PIN_SD_CS)) {
    if (log) log->info("SD mounted");
    return true;
  }
  if (log) log->error("SD mount failed");
  return false;
}

static void serializeWifiKnown(JsonArray arr, const std::vector<WifiKnown>& known) {
  for (const auto& k : known) {
    JsonObject o = arr.createNestedObject();
    o["ssid"] = k.ssid;
    o["pass"] = k.pass;
    o["priority"] = k.priority;
  }
}

static void deserializeWifiKnown(JsonArray arr, std::vector<WifiKnown>& known) {
  known.clear();
  for (JsonObject o : arr) {
    WifiKnown k;
    k.ssid = o["ssid"].as<String>();
    k.pass = o["pass"].as<String>();
    k.priority = o["priority"].as<int>();
    known.push_back(k);
  }
}

bool ConfigManager::save(const Config& in, Logger& log) {
  if (!ensureDir()) return false;
  DynamicJsonDocument doc(1024);
  doc["time"]["tz_offset_minutes"] = in.time.tz_offset_minutes;
  JsonObject gps = doc["gps"].to<JsonObject>();
  gps["uart_rx"] = in.gps.uart_rx;
  gps["uart_tx"] = in.gps.uart_tx;
  gps["baud"] = in.gps.baud;
  gps["rate_hz"] = in.gps.rate_hz;
  gps["dynamic_model"] = in.gps.dynamic_model;
  JsonObject wifi = doc["wifi"].to<JsonObject>();
  wifi["enable"] = in.wifi.enable;
  wifi["connect_attempts"] = in.wifi.connect_attempts;
  JsonArray known = wifi["known"].to<JsonArray>();
  serializeWifiKnown(known, in.wifi.known);
  doc["ui"]["theme"] = in.ui.theme;

  File f = SD.open(CONFIG_FILE, FILE_WRITE);
  if (!f) {
    log.error("Failed to open config for writing");
    return false;
  }
  serializeJsonPretty(doc, f);
  f.close();
  log.info("Config saved");
  return true;
}

bool ConfigManager::load(Config& out, Logger& log) {
  if (!SD.exists(CONFIG_FILE)) {
    log.warn("Config missing, creating default");
    Config def = makeDefault();
    if (!save(def, log)) return false;
    out = def;
    return true;
  }
  File f = SD.open(CONFIG_FILE, FILE_READ);
  if (!f) {
    log.error("Failed to open config file");
    return false;
  }
  DynamicJsonDocument doc(2048);
  DeserializationError err = deserializeJson(doc, f);
  f.close();
  if (err) {
    log.error("Config parse error");
    return false;
  }
  out.time.tz_offset_minutes = doc["time"]["tz_offset_minutes"].as<int>();
  JsonObject gps = doc["gps"];
  out.gps.uart_rx = gps["uart_rx"].as<int>();
  out.gps.uart_tx = gps["uart_tx"].as<int>();
  out.gps.baud = gps["baud"].as<uint32_t>();
  out.gps.rate_hz = gps["rate_hz"].as<int>();
  out.gps.dynamic_model = gps["dynamic_model"].as<String>();
  JsonObject wifi = doc["wifi"];
  out.wifi.enable = wifi["enable"].as<bool>();
  out.wifi.connect_attempts = wifi["connect_attempts"].as<int>();
  deserializeWifiKnown(wifi["known"].as<JsonArray>(), out.wifi.known);
  out.ui.theme = doc["ui"]["theme"].as<String>();
  log.info("Config loaded");
  return true;
}

