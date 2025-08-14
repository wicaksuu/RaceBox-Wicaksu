#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
// Pin definitions derived from README (ESP32-2432S028R)
// ---------------------------------------------------------------------------
#define PIN_TFT_MISO 12
#define PIN_TFT_MOSI 13
#define PIN_TFT_SCLK 14
#define PIN_TFT_CS   15
#define PIN_TFT_DC   2
#define PIN_TFT_RST  -1
#define PIN_TFT_BL   21

#define PIN_TS_IRQ   36
#define PIN_TS_MOSI  32
#define PIN_TS_MISO  39
#define PIN_TS_CLK   25
#define PIN_TS_CS    33

#define PIN_SD_MISO  19
#define PIN_SD_MOSI  23
#define PIN_SD_SCLK  18
#define PIN_SD_CS    5

#define GPS_RX 27
#define GPS_TX 22

// ---------------------------------------------------------------------------
// Scheduler / timing constants
// ---------------------------------------------------------------------------
#define LVGL_TICK_MS        5
#define WIFI_CONNECT_TIMEOUT_MS 10000
#define WIFI_RETRY_DELAY_MS 1000
#define GPS_READ_INTERVAL_MS 5
#define UI_REFRESH_MS       50

// Maximum log lines kept in memory
#define LOGGER_MAX_LINES 100

// Config file paths
#define CONFIG_DIR "/config"
#define CONFIG_FILE CONFIG_DIR "/system.json"

// Forward declarations for structs used across modules
struct Config;
struct GpsStatus;

