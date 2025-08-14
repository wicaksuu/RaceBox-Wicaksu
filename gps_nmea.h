#pragma once
/**
 * @file gps_nmea.h
 * @brief Minimal NMEA sentence helpers for GGA and RMC.
 */

#include "global.h"

namespace NMEA {
  bool parseGGA(const String& line, bool& fix, int& sats, float& hdop);
  bool parseRMC(const String& line, tm& out);
}

