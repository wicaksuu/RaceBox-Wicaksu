/*
  u-blox M10 Race Setup — Serial only (ESP32-ready)
  Fitur:
   - Scan baud -> switch ke target
   - Set rate ke GPS_TARGET_HZ (min 50ms = 20Hz limit HW)
   - Mode dinamis Automotive + 3D-only
   - Output UBX-NAV-PVT @ setiap solusi + NMEA GGA/RMC saja
   - Parser NAV-PVT -> print lat, lon, speed, distance, time UTC+7, altitude, hAcc/vAcc/sAcc

  Sumber key/ID u-blox (M10 SPG 5.20): CFG-NAVSPG-DYNMODEL, FIXMODE, UART1*, MSGOUT-UBX_NAV_PVT_UART1.
*/

#include <Arduino.h>

// ================== PIN & USER CONFIG ==================
#define GPS_RX            27
#define GPS_TX            22
#define GPS_TARGET_BAUD   115200      // Target baud setelah setup
#define GPS_TARGET_HZ     20          // Target rate (Hz). HW M10 min meas 50ms => max ~20Hz
#define TZ_OFFSET_HOURS   +7          // WIB (UTC+7)

// ====== DynModel enum (u-blox NAVSPG DYNMODEL) ======
enum DynModel : uint8_t {
  DM_PORTABLE=0, DM_STATIONARY=2, DM_PEDESTRIAN=3,
  DM_AUTOMOTIVE=4, DM_SEA=5, DM_AIRBORNE_1G=6,
  DM_AIRBORNE_2G=7, DM_AIRBORNE_4G=8
};
#define GPS_DYNMODEL  DM_AUTOMOTIVE   // Race use

// ================== Serial GNSS handle ==================
#if defined(ESP32)
HardwareSerial GPS(1);
#else
#define GPS Serial1
#endif

/************  UBX helpers (VALSET)  ************/
static void flushGPS(){ while (GPS.available()) GPS.read(); }

static void ubxSend(uint8_t cls, uint8_t id, const uint8_t* pl, uint16_t len){
  auto upd=[&](uint8_t x, uint8_t& A, uint8_t& B){ A=(uint8_t)(A+x); B=(uint8_t)(B+A); };
  uint8_t A=0,B=0;
  GPS.write(0xB5); GPS.write(0x62);
  GPS.write(cls);  upd(cls,A,B);
  GPS.write(id);   upd(id,A,B);
  GPS.write((uint8_t)(len & 0xFF)); upd((uint8_t)(len & 0xFF),A,B);
  GPS.write((uint8_t)(len >> 8));   upd((uint8_t)(len >> 8),A,B);
  for(uint16_t i=0;i<len;i++){ GPS.write(pl[i]); upd(pl[i],A,B); }
  GPS.write(A); GPS.write(B); GPS.flush();
}

static bool ubxWaitAck(uint8_t cls, uint8_t id, uint32_t to=600){
  uint32_t t0=millis(); int st=0; uint8_t gotCls=0, gotId=0; bool isAck=false;
  while (millis()-t0 < to){
    if (!GPS.available()) { delay(1); continue; }
    uint8_t b=GPS.read();
    switch(st){
      case 0: st=(b==0xB5)?1:0; break;
      case 1: st=(b==0x62)?2:0; break;
      case 2: if (b==0x05){ st=3; } else st=0; break; // ACK/NAK class
      case 3: isAck=(b==0x01); st=4; break;           // 0x01=ACK, 0x00=NAK
      case 4: st=5; break;                            // len L
      case 5: st=6; break;                            // len H
      case 6: gotCls=b; st=7; break;                  // payload[0]
      case 7: gotId=b; goto done;
    }
  }
done:
  return (gotCls==cls && gotId==id && isAck);
}

// === CFG-VALSET helpers ===
static void putKeyU1(uint8_t*& p, uint32_t key, uint8_t  v){ memcpy(p,&key,4); p+=4; *p++=v; }
static void putKeyU2(uint8_t*& p, uint32_t key, uint16_t v){ memcpy(p,&key,4); p+=4; *p++=(uint8_t)v; *p++=(uint8_t)(v>>8); }
static void putKeyU4(uint8_t*& p, uint32_t key, uint32_t v){ memcpy(p,&key,4); p+=4; memcpy(p,&v,4); p+=4; }
static void putKeyL1(uint8_t*& p, uint32_t key, bool v)    { memcpy(p,&key,4); p+=4; *p++=(uint8_t)(v?1:0); }

static bool sendVALSET_RAM(void (*fill)(uint8_t*&)) {
  uint8_t pay[256]; uint8_t* p=pay;
  *p++=0x00;              // version
  *p++=0x01;              // layer: RAM
  *p++=0x00; *p++=0x00;   // reserved
  fill(p);
  uint16_t L=(uint16_t)(p-pay);
  ubxSend(0x06,0x8A,pay,L);
  return ubxWaitAck(0x06,0x8A,900);
}

/************  Key IDs (M10 SPG 5.20)  ************/
// Rate
#define K_RATE_MEAS              0x30210001u // U2 ms
#define K_RATE_NAV               0x30210002u // U2 cycles
// UART1
#define K_UART1_BAUD             0x40520001u // U4
#define K_UART1IN_UBX            0x10730001u // L1
#define K_UART1IN_NMEA           0x10730002u // L1
#define K_UART1OUT_UBX           0x10740001u // L1
#define K_UART1OUT_NMEA          0x10740002u // L1
// NMEA MSG (UART1)
#define K_MSG_GGA_UART1          0x209100BBu // U1
#define K_MSG_RMC_UART1          0x209100ACu // U1
#define K_MSG_GSA_UART1          0x209100C0u // U1
#define K_MSG_GSV_UART1          0x209100C5u // U1
#define K_MSG_GLL_UART1          0x209100CAu // U1
#define K_MSG_VTG_UART1          0x209100B1u // U1
#define K_MSG_ZDA_UART1          0x209100D9u // U1
// UBX MSG (UART1)
#define K_MSG_UBX_NAV_PVT_UART1  0x20910007u // U1
// NAV config
#define K_NAVSPG_FIXMODE         0x20110011u // E1 (1=2D,2=3D,3=AUTO)
#define K_NAVSPG_DYNMODEL        0x20110021u // E1 (Automotive=4)

// ====== PUBX fallback (enable UBX+NMEA, set baud) ======
static void sendPUBX41(uint32_t baud){
  auto sendNMEA=[&](const char* payload){
    uint8_t c=0; for (const char* p=payload; *p; ++p) c^=(uint8_t)(*p);
    char buf[96]; snprintf(buf,sizeof(buf),"$%s*%02X\r\n",payload,c);
    GPS.print(buf); GPS.flush();
  };
  char pl[64];
  // port 1, in: UBX+NMEA (0003), out: NMEA (0001), baud=<baud>
  snprintf(pl,sizeof(pl),"PUBX,41,1,0003,0001,%u,0",(unsigned)baud);
  sendNMEA(pl);
  delay(150);
#if defined(ESP32)
  GPS.updateBaudRate(baud);
#else
  GPS.begin(baud);
#endif
  delay(150);
}

// ====== NMEA detection for baud scan ======
static bool seeNMEA(uint32_t ms){
  uint32_t t0=millis(); String s;
  while (millis()-t0<ms){
    int c=GPS.read();
    if (c<0){ delay(1); continue; }
    char ch=(char)c;
    if (ch=='\n'){ if (s.startsWith("$GP")||s.startsWith("$GN")) return true; s=""; }
    else if (ch!='\r') s+=ch;
  }
  return false;
}

static const uint32_t SCAN_BAUDS[] = {9600, 38400, 57600, 115200, 230400};
static bool scanBaud(uint32_t &found){
  for (uint32_t b: SCAN_BAUDS){
#if defined(ESP32)
    GPS.begin(b, SERIAL_8N1, GPS_RX, GPS_TX);
#else
    GPS.begin(b);
#endif
    delay(150); flushGPS();
    if (seeNMEA(600)){ found=b; return true; }
  }
  return false;
}

// ====== Apply race config at current baud ======
static bool applyRaceConfig(uint16_t measMs, DynModel dm){
  bool ok = sendVALSET_RAM([&](uint8_t*& p){
    // rate
    if (measMs < 50) measMs = 50;          // M10 min 50 ms
    putKeyU2(p, K_RATE_MEAS, measMs);
    putKeyU2(p, K_RATE_NAV , 1);

    // protocol enable (in/out)
    putKeyL1(p, K_UART1IN_UBX , 1);
    putKeyL1(p, K_UART1IN_NMEA, 1);
    putKeyL1(p, K_UART1OUT_UBX, 1);
    putKeyL1(p, K_UART1OUT_NMEA,1);

    // NMEA minimal: GGA & RMC ON (1), yang lain OFF (0)
    putKeyU1(p, K_MSG_GGA_UART1, 1);
    putKeyU1(p, K_MSG_RMC_UART1, 1);
    putKeyU1(p, K_MSG_GSA_UART1, 0);
    putKeyU1(p, K_MSG_GSV_UART1, 0);
    putKeyU1(p, K_MSG_GLL_UART1, 0);
    putKeyU1(p, K_MSG_VTG_UART1, 0);
    putKeyU1(p, K_MSG_ZDA_UART1, 0);

    // UBX PVT periodic (1 = setiap solusi)
    putKeyU1(p, K_MSG_UBX_NAV_PVT_UART1, 1);

    // NAV engine: mode 3D-only + Automotive
    putKeyU1(p, K_NAVSPG_FIXMODE , 2);           // 3D-only
    putKeyU1(p, K_NAVSPG_DYNMODEL, (uint8_t)dm); // Automotive
  });
  Serial.printf("  [ACK] VALSET race config: %s\r\n", ok?"OK":"FAIL");
  return ok;
}

// ====== Switch baud ======
static bool switchToBaud(uint32_t target){
  bool okB = sendVALSET_RAM([&](uint8_t*& p){ putKeyU4(p, K_UART1_BAUD, target); });
  Serial.printf("  [ACK] Set BAUD %u (VALSET): %s\r\n", (unsigned)target, okB?"OK":"FAIL");
  delay(120);
#if defined(ESP32)
  GPS.updateBaudRate(target);
#else
  GPS.begin(target);
#endif
  delay(150);
  if (!seeNMEA(700)){
    Serial.println("  [FALLBACK] $PUBX,41 …");
    sendPUBX41(target);
    if (!seeNMEA(700)){
      Serial.println("  [ERR] NMEA belum terlihat @target baud.");
      return false;
    }
  }
  Serial.printf("  [OK] NMEA aktif @%u.\r\n", (unsigned)target);
  return true;
}

// ====== Orchestrator ======
static void gpsAutoSetupRace(uint32_t targetBaud, uint16_t targetHz, DynModel dm){
  uint16_t measMs = (uint16_t)max(50, (int)(1000.0f/targetHz + 0.5f));
  Serial.printf("[GPS] Auto-detect → %u baud, %u Hz (meas=%ums), mode=Automotive, 3D-only…\r\n",
                (unsigned)targetBaud, (unsigned)targetHz, (unsigned)measMs);

  uint32_t cur=0;
  if (!scanBaud(cur)) { Serial.println("  [ERR] NMEA tidak ditemukan. Cek wiring/power."); return; }
  Serial.printf("  [OK] NMEA terdeteksi @ %u bps\r\n", (unsigned)cur);

  if (cur != targetBaud) {
    Serial.println("  [STEP] Switch ke baud target…");
    if (!switchToBaud(targetBaud)) { Serial.println("  [FAIL] Gagal switch baud."); return; }
  } else {
#if defined(ESP32)
    GPS.updateBaudRate(targetBaud);
#else
    GPS.begin(targetBaud);
#endif
    delay(120);
  }

  Serial.println("  [STEP] Apply race config (rate/protocol/msgs/nav)…");
  applyRaceConfig(measMs, dm);
}

// ================== UBX-NAV-PVT parser ==================
struct PVT {
  uint32_t iTOW;    // ms
  int32_t  lon;     // 1e-7 deg
  int32_t  lat;     // 1e-7 deg
  int32_t  height;  // mm (ellipsoid)
  int32_t  hMSL;    // mm
  uint32_t hAcc;    // mm
  uint32_t vAcc;    // mm
  int32_t  velN, velE, velD; // mm/s
  uint32_t gSpeed;  // mm/s
  uint32_t sAcc;    // mm/s
  uint16_t pDOP;    // 0.01
  uint8_t  fixType; // 3=3D, 4=GNSS+DR
  uint8_t  numSV;
  uint16_t year; uint8_t month, day, hour, min, sec;
  bool valid;
};

static uint8_t ubxBuf[128];
static int     ubxPos=0, ubxLen=0;
static uint8_t ckA=0, ckB=0;
static uint8_t stateU=0, clsU=0, idU=0, lenL=0, lenH=0;
static PVT lastPVT; static bool gotPVT=false;

static void resetUBX(){ stateU=ubxPos=0; ckA=ckB=0; ubxLen=0; }

static void feedUBX(uint8_t b){
  switch(stateU){
    case 0: stateU = (b==0xB5)?1:0; break;
    case 1: stateU = (b==0x62)?2:0; break;
    case 2: clsU=b; ckA=ckB=0; ckA+=b; ckB+=ckA; stateU=3; break;
    case 3: idU=b;  ckA+=b; ckB+=ckA; stateU=4; break;
    case 4: lenL=b; ckA+=b; ckB+=ckA; stateU=5; break;
    case 5: lenH=b; ckA+=b; ckB+=ckA; ubxLen = (lenH<<8)|lenL; ubxPos=0; stateU=6; break;
    case 6:
      if (ubxPos < (int)sizeof(ubxBuf)) ubxBuf[ubxPos]=b;
      ubxPos++;
      ckA+=b; ckB+=ckA;
      if (ubxPos >= ubxLen) stateU=7;
      break;
    case 7:
      if (b==ckA) stateU=8; else resetUBX();
      break;
    case 8:
      if (b==ckB){
        // Completed frame
        if (clsU==0x01 && idU==0x07 && ubxLen>=92){
          // parse PVT (subset)
          PVT p={0};
          const uint8_t* q=ubxBuf;
          auto U4=[&](){ uint32_t v= q[0]|(q[1]<<8)|(q[2]<<16)|(q[3]<<24); q+=4; return v; };
          auto I4=[&](){ int32_t  v= (int32_t)(q[0]|(q[1]<<8)|(q[2]<<16)|(q[3]<<24)); q+=4; return v; };
          auto U2=[&](){ uint16_t v= q[0]|(q[1]<<8); q+=2; return v; };
          auto U1=[&](){ return *q++; };

          p.iTOW = U4();
          uint16_t year = U2(); uint8_t month=U1(), day=U1(), hour=U1(), min=U1(), sec=U1(); q+=1; // valid flags skip 1
          q+=4; // tAcc
          p.fixType = U1(); q+=1; // flags
          p.numSV   = U1(); q+=1; // flags2
          p.lon = I4(); p.lat = I4();
          p.height = I4(); p.hMSL = I4();
          p.hAcc = U4(); p.vAcc = U4();
          p.velN = I4(); p.velE = I4(); p.velD = I4();
          p.gSpeed = U4(); q+=4; // headMot
          p.sAcc = U4();
          p.pDOP = U2();
          p.year=year; p.month=month; p.day=day; p.hour=hour; p.min=min; p.sec=sec;
          p.valid = (p.fixType>=3);
          lastPVT = p; gotPVT=true;
        }
      }
      resetUBX();
      break;
  }
}

// ================== Distance + time utils ==================
static double deg2rad(double d){ return d*3.14159265358979323846/180.0; }
static double haversine_m(double lat1, double lon1, double lat2, double lon2){
  double R=6371000.0;
  double dLat=deg2rad(lat2-lat1), dLon=deg2rad(lon2-lon1);
  double a=sin(dLat/2)*sin(dLat/2) + cos(deg2rad(lat1))*cos(deg2rad(lat2))*sin(dLon/2)*sin(dLon/2);
  double c=2*atan2(sqrt(a), sqrt(1-a));
  return R*c;
}
static double lastLat=0,lastLon=0,totalDist=0; static bool havePrev=false;

// ================== Arduino setup/loop ==================
void setup(){
  Serial.begin(115200);
  delay(200);
#if defined(ESP32)
  GPS.begin(9600, SERIAL_8N1, GPS_RX, GPS_TX); // start low for scan
#else
  GPS.begin(9600);
#endif
  delay(200);

  gpsAutoSetupRace(GPS_TARGET_BAUD, GPS_TARGET_HZ, GPS_DYNMODEL);
  Serial.println("[GPS] Setup selesai. Menunggu NAV-PVT…");
}

void loop(){
  while (GPS.available()){
    uint8_t b=(uint8_t)GPS.read();
    // Feed UBX parser
    feedUBX(b);
    // (opsional) forward NMEA ke Serial — nonaktifkan jika noise
    // if (b=='$' || b=='\n' || b=='\r' || (b>=' ' && b<='~')) Serial.write(b);
  }

  if (gotPVT){
    gotPVT=false;
    if (lastPVT.valid){
      double lat = lastPVT.lat * 1e-7;
      double lon = lastPVT.lon * 1e-7;
      if (havePrev){
        totalDist += haversine_m(lastLat,lastLon,lat,lon);
      } else {
        havePrev=true;
      }
      lastLat=lat; lastLon=lon;

      // speed (km/h)
      double spd_mps = lastPVT.gSpeed * 0.001;     // mm/s -> m/s
      double spd_kmh = spd_mps * 3.6;

      // altitude (MSL meters)
      double alt_m = lastPVT.hMSL * 0.001;

      // accuracy
      double hAcc_m = lastPVT.hAcc * 0.001;
      double vAcc_m = lastPVT.vAcc * 0.001;
      double sAcc_mps = lastPVT.sAcc * 0.001;

      // Local time UTC+7
      int hour = (int)lastPVT.hour + TZ_OFFSET_HOURS;
      int day=lastPVT.day, month=lastPVT.month, year=lastPVT.year;
      if (hour>=24){ hour-=24; day++; /* naive roll; cukup untuk display live */ }
      if (hour<0){ hour+=24; day--; }

      Serial.printf(
        "PVT | lat=%.7f lon=%.7f | v=%.2f km/h | dist=%.1f m | alt=%.1f m | acc(h=%.2f m, v=%.2f m, s=%.2f m/s) | sats=%u fix=%u | %04u-%02u-%02u %02u:%02u:%02u WIB\r\n",
        lat, lon, spd_kmh, totalDist, alt_m, hAcc_m, vAcc_m, sAcc_mps,
        lastPVT.numSV, lastPVT.fixType, year, month, day, hour, lastPVT.min, lastPVT.sec
      );
    }
  }
}
