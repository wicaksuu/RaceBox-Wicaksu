#pragma once
/**
 * @file gps_nmea.h
 * @brief Minimal NMEA sentence helpers for GGA and RMC.
 */

#include "global.h"

namespace NMEA {
  /**
   * @brief Parse a GGA sentence to extract fix flag, satellite count, HDOP, and altitude.
   */
  bool parseGGA(const String& line, bool& fix, int& sats, float& hdop, float& alt);

  /**
   * @brief Parse an RMC sentence to populate a tm structure with date and time.
   */
  bool parseRMC(const String& line, tm& out);
}

