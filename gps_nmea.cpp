#include "gps_nmea.h"

static std::vector<String> split(const String& s, char delim) {
  std::vector<String> out; String token; for (size_t i=0;i<s.length();++i){
    char c=s[i];
    if(c==delim){ out.push_back(token); token=""; }
    else token+=c;
  }
  out.push_back(token);
  return out;
}

bool NMEA::parseGGA(const String& line, bool& fix, int& sats, float& hdop, float& alt) {
  auto fields = split(line, ',');
  if(fields.size() < 10) return false; // need altitude field
  int fixQ = fields[6].toInt();
  fix = fixQ>0;
  sats = fields[7].toInt();
  hdop = fields[8].toFloat();
  alt = fields[9].toFloat();
  return true;
}

bool NMEA::parseRMC(const String& line, tm& out) {
  auto fields = split(line, ',');
  if(fields.size() < 10) return false;
  String time = fields[1];
  String date = fields[9];
  if (time.length() < 6 || date.length() < 6) return false;
  out.tm_hour = time.substring(0,2).toInt();
  out.tm_min  = time.substring(2,4).toInt();
  out.tm_sec  = time.substring(4,6).toInt();
  out.tm_mday = date.substring(0,2).toInt();
  out.tm_mon  = date.substring(2,4).toInt() - 1; // tm_mon 0-11
  out.tm_year = date.substring(4,6).toInt() + 100; // years since 1900
  return true;
}

