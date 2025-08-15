#pragma once
#include <Arduino.h>
#include <vector>
#include "gps_read.h"

// ===== File lokasi =====
inline constexpr const char* RACE_PATH = "/config/race.json";

// ===== Konfigurasi race =====
struct Trap {
  String name;     // "60ft", "1/8mi", ...
  float  at_m;     // posisi dari start (meter)
  float  window_m; // panjang window utk trap speed avg (pusat di at_m), boleh 0 = nonaktif
};

struct RaceConfig {
  // Start arming/trigger
  float arm_speed_kph      = 1.0f;  // siap start jika kecepatan > ini
  float trigger_speed_kph  = 5.0f;  // mulai timing jika > ini (rising)
  float max_hdop_m         = 1.5f;  // gating fix
  // Default daftar traps (drag)
  std::vector<Trap> traps = {
    {"60ft",   18.288f, 2.0f},
    {"330ft", 100.584f, 5.0f},
    {"1/8mi", 201.168f, 10.0f},
    {"1000ft",304.800f, 10.0f},
    {"1/4mi", 402.336f, 20.0f}
  };
};

bool race_load(RaceConfig& out);           // dari SD (isi default jika tidak ada)
bool race_save(const RaceConfig& cfg);     // ke SD
RaceConfig& race_cfg();                    // akses global

// ===== Runtime race manager =====
struct TrapResult {
  String name;
  float  at_m;
  bool   crossed;
  uint32_t t_start_ms;  // waktu start
  uint32_t t_cross_ms;  // waktu crossing
  float   et_ms;        // t_cross - t_start (ms)
  float   trap_kph;     // avg speed di window (kalau window_m>0)
};

struct RaceState {
  bool armed = false;
  bool running = false;
  uint32_t t_arm_ms = 0;
  uint32_t t_start_ms = 0;
  double lat0=0, lon0=0; // titik start
  double last_lat=0, last_lon=0;
  float  cum_dist_m = 0; // jarak dari start (path length)
  std::vector<TrapResult> results;
};

void race_begin();                            // reset state & siapkan buffer
void race_arm(bool on);                       // arm/disarm manual
void race_reset();                            // reset penuh (hasil hilang)
void race_update(const GPSFix& fix);          // panggil tiap ada fix baru valid
const RaceState& race_state();                // baca state
