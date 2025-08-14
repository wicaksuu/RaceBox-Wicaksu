#pragma once
/**
 * @file global.h
 * @brief Global pin definitions, configuration structures, and common includes.
 */

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include <vector>
#include <functional>

// =================== Pinout (from README) ===================
#define PIN_TFT_MISO    12
#define PIN_TFT_MOSI    13
#define PIN_TFT_SCLK    14
#define PIN_TFT_CS      15
#define PIN_TFT_DC       2
#define PIN_TFT_RST     -1
#define PIN_TFT_BACKLIGHT 21

// Touchscreen (XPT2046 on VSPI)
#define PIN_TS_IRQ      36
#define PIN_TS_MOSI     32
#define PIN_TS_MISO     39
#define PIN_TS_CLK      25
#define PIN_TS_CS       33

// microSD (VSPI)
#define PIN_SD_CS        5

// GPS UART
#define GPS_RX          27
#define GPS_TX          22

// LVGL timing
#define LVGL_TICK_MS     5

// =================== Config structures ===================
/** @brief Time configuration */
struct TimeCfg {
  int tz_offset_minutes = 0;  //!< Local timezone offset from UTC in minutes
};

/** @brief GPS configuration */
struct GpsCfg {
  int uart_rx = GPS_RX;       //!< UART RX pin
  int uart_tx = GPS_TX;       //!< UART TX pin
  uint32_t baud = 115200;     //!< GPS baud rate
  int rate_hz = 10;           //!< Target update rate (Hz)
  String dynamic_model = "automotive"; //!< u-blox dynamic model
};

/** @brief Known Wi-Fi network */
struct WifiKnown {
  String ssid;                //!< SSID
  String pass;                //!< Password
  int priority = 0;           //!< Higher value = higher priority
};

/** @brief Wi-Fi configuration */
struct WifiCfg {
  bool enable = true;                     //!< Enable Wi-Fi module
  std::vector<WifiKnown> known;           //!< Known networks
  int connect_attempts = 5;               //!< Attempts per SSID
};

/** @brief UI configuration */
struct UiCfg {
  String theme = "racing-dark"; //!< Theme name
};

/** @brief System configuration root */
struct Config {
  TimeCfg time;    //!< Time settings
  GpsCfg gps;      //!< GPS settings
  UiCfg ui;        //!< UI settings
  WifiCfg wifi;    //!< Wi-Fi settings
};

// =================== Utility constants ===================
#define WIFI_CONNECT_TIMEOUT_MS 15000UL
#define GPS_POLL_INTERVAL_MS     5UL
#define WIFI_SCAN_INTERVAL_MS  1000UL

