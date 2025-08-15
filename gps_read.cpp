#include "gps_read.h"
#include "global.h"
#include "logview.h"
#include <math.h>

// ====== Serial alias ======
#define GPS GPSSerial

static GPSFilterTuning T;
static GPSStats S;

// Ring buffer byte -> line parser
static String line;            // 1 kalimat NMEA
static uint16_t LINE_MAX = 128;

// Last raw (dari RMC/GGA)
static bool haveRMC=false, haveGGA=false;
static double raw_lat=0, raw_lon=0;  // deg
static float  raw_sog=0, raw_cog=0;  // m/s, deg
static float  raw_alt=0, raw_hdop=999;
static uint8_t raw_sv=0, raw_fixQ=0;

// Filtered state
static bool filt_init=false;
static float sog_prev=0, sog_filt=0;  // m/s
static float a_prev=0;                 // m/s^2
static uint32_t t_prev=0;

static inline uint8_t hex2(byte c){ return (c>='A')?(c-'A'+10):((c>='a')?(c-'a'+10):(c-'0')); }

// deg min.mmmm -> deg
static double dm_to_deg(double dm){
  double d = floor(dm/100.0);
  double m = dm - d*100.0;
  return d + m/60.0;
}

static bool nmea_cksum_ok(const char* s){
  // s: pointer ke char setelah '$', sampai sebelum '*'
  uint8_t x=0; while (*s && *s!='*'){ x ^= (uint8_t)*s++; }
  if (*s!='*') return false; s++;
  if (!s[0]||!s[1]) return false;
  uint8_t h = (hex2(s[0])<<4) | hex2(s[1]);
  return x==h;
}

static bool parse_gga(const String& l){
  // $GxGGA,time,lat,N,lon,E,fix,sv,hdop,alt,M,....*cs
  // cepat: pakai strtok manual
  char buf[160]; size_t n = min(sizeof(buf)-1, (size_t)l.length()); l.toCharArray(buf, n+1);
  if (!nmea_cksum_ok(buf+1)) { S.cks_fail++; return false; }
  // tokenizing
  int fld=0; char* p = buf; char* tok=nullptr;
  while (*p && *p!='$') p++; if (*p=='$') p++;
  // ubah '*' jadi ',' biar gampang
  for (char* q=p; *q; ++q) if (*q=='*') *q=',';
  double lat_dm=0, lon_dm=0; char latH='N', lonH='E';
  int fixQ=0, sv=0; float hdop=99, alt=0;
  while ((tok = strsep(&p, ","))){
    switch(fld++){
      case 2: lat_dm = atof(tok); break;
      case 3: latH   = tok[0];    break;
      case 4: lon_dm = atof(tok); break;
      case 5: lonH   = tok[0];    break;
      case 6: fixQ   = atoi(tok); break;
      case 7: sv     = atoi(tok); break;
      case 8: hdop   = atof(tok); break;
      case 9: alt    = atof(tok); break;
    }
  }
  if (lat_dm<=0 || lon_dm<=0) return false;
  double lat = dm_to_deg(lat_dm) * (latH=='S'?-1:1);
  double lon = dm_to_deg(lon_dm) * (lonH=='W'?-1:1);
  raw_lat=lat; raw_lon=lon; raw_alt=alt; raw_hdop=hdop; raw_sv=sv; raw_fixQ=fixQ;
  haveGGA=true; S.gga_ok++; return true;
}

static bool parse_rmc(const String& l){
  // $GxRMC,time,status,lat,N,lon,E,speed(kn),cog, date, ... *cs
  char buf[180]; size_t n = min(sizeof(buf)-1, (size_t)l.length()); l.toCharArray(buf, n+1);
  if (!nmea_cksum_ok(buf+1)) { S.cks_fail++; return false; }
  int fld=0; char* p=buf; char* tok=nullptr; for(char* q=p; *q; ++q) if (*q=='*') *q=',';
  double lat_dm=0, lon_dm=0; char latH='N', lonH='E'; char status='V';
  float sp_kn=0, cog=0;
  while ((tok = strsep(&p, ","))){
    switch(fld++){
      case 2: status = tok[0]; break; // A=valid
      case 3: lat_dm = atof(tok); break;
      case 4: latH   = tok[0];    break;
      case 5: lon_dm = atof(tok); break;
      case 6: lonH   = tok[0];    break;
      case 7: sp_kn  = atof(tok); break;
      case 8: cog    = atof(tok); break;
    }
  }
  if (status!='A') return false;
  if (lat_dm<=0 || lon_dm<=0) return false;
  double lat = dm_to_deg(lat_dm) * (latH=='S'?-1:1);
  double lon = dm_to_deg(lon_dm) * (lonH=='W'?-1:1);
  raw_lat=lat; raw_lon=lon; raw_cog=cog; raw_sog = sp_kn * 0.514444f; // kn->m/s
  haveRMC=true; S.rmc_ok++; return true;
}

// Haversine (meter)
static double dist_haversine_m(double lat1,double lon1,double lat2,double lon2){
  static constexpr double R = 6371000.0;
  double r1=lat1*M_PI/180.0, r2=lat2*M_PI/180.0, dlat=(lat2-lat1)*M_PI/180.0, dlon=(lon2-lon1)*M_PI/180.0;
  double a = sin(dlat/2)*sin(dlat/2) + cos(r1)*cos(r2)*sin(dlon/2)*sin(dlon/2);
  double c = 2*atan2(sqrt(a), sqrt(1-a));
  return R*c;
}

// median of 3
static inline float med3(float a, float b, float c){
  if (a>b) swap(a,b); if (b>c) swap(b,c); if (a>b) swap(a,b); return b;
}

void gps_reader_begin(uint16_t lineBuf){
  LINE_MAX = max<uint16_t>(lineBuf, 96);
  line.reserve(LINE_MAX);
  haveGGA=haveRMC=false;
  filt_init=false;
  S = GPSStats{};
}

void gps_set_filter_tuning(const GPSFilterTuning& t){ T = t; }

const GPSStats& gps_stats(){ return S; }

bool gps_poll(GPSFix& out){
  // Baca per loop semua byte yang ada, pecah per-line
  bool any=false;
  while (GPSSerial.available()){
    int c = GPSSerial.read();
    if (c<0) break;
    char ch = (char)c;
    if (ch=='\r') continue;
    if (ch=='\n'){
      if (line.length()>5 && line[0]=='$'){
        S.nmea_lines++;
        if (line.indexOf("GGA")>=0) parse_gga(line);
        else if (line.indexOf("RMC")>=0) parse_rmc(line);
        // Reset jika terlalu panjang
        if (line.length()>LINE_MAX) line.remove(0);
      }
      line = "";
    } else {
      if (line.length() < LINE_MAX) line += ch;
      else line = ""; // overflow guard
    }
    any=true;
  }
  if (!any) return false;

  // Bila punya RMC atau GGA terbaru → buat fix gabungan
  if (!(haveGGA || haveRMC)) return false;

  uint32_t tnow = millis();

  // ===== Gating kualitas =====
  bool quality_ok = (raw_fixQ>0) && (raw_hdop<=T.max_hdop_m);
  if (!quality_ok){
    S.reject_hdop++;
    out = {raw_lat, raw_lon, raw_alt, sog_filt, raw_cog, raw_hdop, raw_fixQ, raw_sv, tnow, false};
    return true; // laporkan juga sbg "invalid" untuk UI
  }

  // ===== Filter kecepatan =====
  float dt = (tnow - t_prev) * 0.001f;
  dt = dt > 0 ? dt : 0.05f;

  // median-of-3 (raw trio: prev_raw ~ sog_prev, raw_sog, pred)
  float pred = sog_prev; // prediksi sederhana
  float sog_med = med3(sog_prev, raw_sog, pred);

  // accel & jerk
  float a = (sog_med - sog_prev) / max(dt, 1e-3f);
  a = constrain(a, -T.max_accel_mps2, T.max_accel_mps2);
  float jerk = (a - a_prev) / max(dt, 1e-3f);
  // adaptive alpha  (jerk tinggi → kecil alpha, smoothing lebih besar)
  float j = fabsf(jerk);
  float alpha = T.ema_alpha_max - (T.ema_alpha_max-T.ema_alpha_min) * (j / (j + T.max_jerk_mps3));
  alpha = constrain(alpha, T.ema_alpha_min, T.ema_alpha_max);

  if (!filt_init){ sog_filt = raw_sog; filt_init=true; }
  else           { sog_filt = sog_filt + alpha * (sog_med - sog_filt); }

  // outlier clamp terhadap lonjakan tak wajar
  if (fabsf(jerk) > (T.max_jerk_mps3*3.0f)) {
    S.reject_jump++;
    // tahan filter (jangan update); biarkan kembali stabil next samples
    sog_filt = sog_prev;
  }

  // Update memory
  sog_prev = sog_filt;
  a_prev   = a;
  t_prev   = tnow;

  out = {raw_lat, raw_lon, raw_alt, sog_filt, raw_cog, raw_hdop, raw_fixQ, raw_sv, tnow, true};
  return true;
}
