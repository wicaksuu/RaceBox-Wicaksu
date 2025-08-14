#pragma once
/**
 * @file gps_mgr.h
 * @brief Manage u-blox GPS and provide status information.
 */

#include "global.h"
#include "logger.h"
#include "gps_nmea.h"

struct GpsStatus {
  bool hasFix = false; //!< True if a 2D/3D fix is available
  float hdop = 99.0f;  //!< Horizontal dilution of precision
  float acc_m = 0.0f;  //!< Approximate horizontal accuracy in meters
  int sats = 0;        //!< Number of locked satellites
  float rate_hz = 0.0f;//!< Measured update rate
  time_t utc = 0;      //!< Last UTC time from RMC
};

class GpsMgr {
public:
  bool begin(const GpsCfg& cfg, Logger& log);
  void tick(uint32_t now);
  GpsStatus status() const { return _status; }
private:
  void processLine(const String& line);
  HardwareSerial* _ser = nullptr;
  Logger* _log = nullptr;
  GpsCfg _cfg;
  String _buf;
  GpsStatus _status;
  uint32_t _startMs = 0;
  uint32_t _msgCount = 0;
};

#endif
