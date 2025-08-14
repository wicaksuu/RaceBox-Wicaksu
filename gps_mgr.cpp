#include "gps_mgr.h"

/** @brief Initialize GPS serial and send basic configuration. */
bool GpsMgr::begin(const GpsCfg& cfg, Logger& log) {
  _cfg = cfg;
  _log = &log;
  _ser = &Serial1; // using UART1 on ESP32
  _ser->begin(9600, SERIAL_8N1, cfg.uart_rx, cfg.uart_tx);
  log.info("GPS serial started at 9600");
  // TODO: auto-baud detection and UBX configuration
  _ser->flush();
  _ser->updateBaudRate(cfg.baud);
  log.info("GPS baud set to %u", cfg.baud);
  _startMs = millis();
  _msgCount = 0;
  return true;
}

/** @brief Periodic polling of GPS data and NMEA parsing. */
void GpsMgr::tick(uint32_t now) {
  while (_ser && _ser->available()) {
    char c = (char)_ser->read();
    if (c == '\n') {
      processLine(_buf);
      _buf = "";
    } else if (c != '\r') {
      _buf += c;
    }
  }
  if (now - _startMs >= 1000) {
    _status.rate_hz = _msgCount * 1000.0f / (now - _startMs);
    _msgCount = 0;
    _startMs = now;
  }
}

void GpsMgr::processLine(const String& line) {
  if (line.startsWith("$GPGGA") || line.startsWith("$GNGGA")) {
    bool fix; int sats; float hdop; float alt;
    if (NMEA::parseGGA(line, fix, sats, hdop, alt)) {
      _status.hasFix = fix;
      _status.sats = sats;
      _status.hdop = hdop;
      _status.alt_m = alt;
      _status.acc_m = hdop * 5.0f; // rough estimate of accuracy
      _msgCount++;
    }
  } else if (line.startsWith("$GPRMC") || line.startsWith("$GNRMC")) {
    tm t = {};
    if (NMEA::parseRMC(line, t)) {
      _status.utc = mktime(&t);
    }
  }
}

