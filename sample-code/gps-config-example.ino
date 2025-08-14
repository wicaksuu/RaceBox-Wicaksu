// ================== PIN & CONFIG ==================
#define GPS_RX 27
#define GPS_TX 22
#define GPS_BAUD 115200

/************  AUTO BAUD + 20 Hz SETUP (u-blox M10)  ************/

// ===== On-screen logger untuk GPS setup =====

// >>> Tambahkan ini:
TFT_eSprite sprGpsSetup = TFT_eSprite(&tft);

static const int GPSLOG_MAX = 12;
static String gpsLogLines[GPSLOG_MAX];
static int gpsLogCount = 0;

static void gpsSetupRender() {
  sprGpsSetup.fillSprite(COLOR_BG);
  // Title
  sprGpsSetup.setTextFont(2);
  sprGpsSetup.setTextColor(COLOR_PRIMARY, COLOR_BG);
  sprGpsSetup.setCursor(8, 6);
  sprGpsSetup.print("GPS Setup");

  // Garis pemisah
  sprGpsSetup.drawFastHLine(8, 24, SCREEN_WIDTH - 16, COLOR_DIM);

  // Isi log
  sprGpsSetup.setTextFont(1);
  sprGpsSetup.setTextColor(COLOR_TEXT, COLOR_BG);
  int y = 30;
  int start = gpsLogCount > GPSLOG_MAX ? (gpsLogCount - GPSLOG_MAX) : 0;
  for (int i = start; i < gpsLogCount; ++i) {
    int idx = i % GPSLOG_MAX;
    sprGpsSetup.setCursor(8, y);
    sprGpsSetup.print(gpsLogLines[idx]);
    y += 12;
  }
  sprGpsSetup.pushSprite(0, 0);
}

static void gpsSetupStart() {
  sprGpsSetup.createSprite(SCREEN_WIDTH, SCREEN_HEIGHT);
  gpsLogCount = 0;
  gpsSetupRender();
}

static void gpsSetupDone() {
  sprGpsSetup.deleteSprite();
}

static void gpsLogf(const char* fmt, ...) {
  char buf[160];
  va_list ap; va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);

  // Mirror ke Serial:
  Serial.println(buf);

  // Simpan ke buffer & render
  gpsLogLines[gpsLogCount % GPSLOG_MAX] = String(buf);
  gpsLogCount++;
  gpsSetupRender();
}


// ====== BAUD scan list ======
static const uint32_t BAUDS[] = {9600, 38400, 57600, 115200, 230400};

// --- util kecil
static void flushGPS() { while (GPS.available()) GPS.read(); }

// Deteksi ada NMEA (“$GP…/$GN…”) dalam ms tertentu
static bool seeNMEA(uint32_t ms) {
  uint32_t t0 = millis(); int lines=0; String s;
  while (millis()-t0 < ms) {
    int c = GPS.read();
    if (c < 0) { delay(1); continue; }
    char ch=(char)c;
    if (ch=='\n') { if (s.startsWith("$GP") || s.startsWith("$GN")) { lines++; if (lines>=1) return true; } s=""; }
    else if (ch!='\r') s+=ch;
  }
  return false;
}

// Scan beberapa baud sampai ketemu NMEA
static bool scanBaud(uint32_t &found) {
  for (uint32_t b: BAUDS) {
    GPS.begin(b, SERIAL_8N1, GPS_RX, GPS_TX);
    delay(150);
    flushGPS();
    if (seeNMEA(600)) { found = b; return true; }
  }
  return false;
}

void ubxSend(uint8_t cls, uint8_t id, const uint8_t* pl, uint16_t len){
  auto upd=[&](uint8_t x, uint8_t& A, uint8_t& B){ A = (uint8_t)(A + x); B = (uint8_t)(B + A); };
  uint8_t A=0,B=0;
  GPS.write(0xB5); GPS.write(0x62);
  GPS.write(cls);  upd(cls,A,B);
  GPS.write(id);   upd(id,A,B);
  GPS.write((uint8_t)(len & 0xFF)); upd((uint8_t)(len & 0xFF),A,B);
  GPS.write((uint8_t)(len >> 8));   upd((uint8_t)(len >> 8),A,B);
  for (uint16_t i=0;i<len;i++){ GPS.write(pl[i]); upd(pl[i],A,B); }
  GPS.write(A); GPS.write(B); GPS.flush();
}
bool ubxWaitAck(uint8_t cls, uint8_t id, uint32_t to=500){
  uint32_t t0=millis(); int st=0; uint8_t gotCls=0, gotId=0; bool isAck=false;
  while (millis()-t0 < to){
    if (!GPS.available()) { delay(1); continue; }
    uint8_t b=GPS.read();
    switch(st){
      case 0: st=(b==0xB5)?1:0; break;
      case 1: st=(b==0x62)?2:0; break;
      case 2: if (b==0x05){ st=3; } else st=0; break;               // ACK/NAK class
      case 3: isAck=(b==0x01); st=4; break;                         // 0x01=ACK,0x00=NAK
      case 4: st=5; break;                                          // len L
      case 5: st=6; break;                                          // len H
      case 6: gotCls=b; st=7; break;                                // payload[0]
      case 7: gotId=b; goto done;
    }
  }
done:
  return (gotCls==cls && gotId==id && isAck);
}
// ===== VALSET (pakai ubxSend/ubxWaitAck milikmu) =====
static void putKeyU1(uint8_t*& p, uint32_t key, uint8_t  v){ memcpy(p,&key,4); p+=4; *p++=v; }
static void putKeyU2(uint8_t*& p, uint32_t key, uint16_t v){ memcpy(p,&key,4); p+=4; *p++=(uint8_t)v; *p++=(uint8_t)(v>>8); }
static void putKeyU4(uint8_t*& p, uint32_t key, uint32_t v){ memcpy(p,&key,4); p+=4; memcpy(p,&v,4); p+=4; }
static void putKeyL1(uint8_t*& p, uint32_t key, bool v)    { memcpy(p,&key,4); p+=4; *p++=(uint8_t)(v?1:0); }

static bool sendVALSET_RAM(void (*fill)(uint8_t*&)) {
  uint8_t pay[128]; uint8_t* p=pay;
  *p++=0x00;              // version
  *p++=0x01;              // layer RAM
  *p++=0x00; *p++=0x00;   // reserved
  fill(p);
  uint16_t L = (uint16_t)(p - pay);
  ubxSend(0x06,0x8A,pay,L);
  return ubxWaitAck(0x06,0x8A,800);
}

// ===== Keys (M10) =====
#define K_RATE_MEAS        0x30210001u // U2 ms
#define K_RATE_NAV         0x30210002u // U2 cycles
#define K_UART1_BAUD       0x40520001u // U4
#define K_UART1IN_UBX      0x10730001u // L1
#define K_UART1IN_NMEA     0x10730002u // L1
#define K_UART1OUT_NMEA    0x10740002u // L1
#define K_MSG_GGA_UART1    0x209100BBu // U1
#define K_MSG_RMC_UART1    0x209100ACu // U1
#define K_MSG_GSA_UART1    0x209100C0u // U1
#define K_MSG_GSV_UART1    0x209100C5u // U1
#define K_MSG_GLL_UART1    0x209100CAu // U1
#define K_MSG_VTG_UART1    0x209100B1u // U1
#define K_MSG_ZDA_UART1    0x209100D9u // U1

// ===== Fallback: enable UBX input via NMEA $PUBX,41 =====
static void sendPUBX41(uint32_t baud){
  auto sendNMEA=[&](const char* payload){
    uint8_t c=0; for (const char* p=payload; *p; ++p) c^=(uint8_t)(*p);
    char buf[96]; snprintf(buf,sizeof(buf),"$%s*%02X\r\n",payload,c);
    GPS.print(buf); GPS.flush();
  };
  char pl[64];
  // $PUBX,41,1,0003,0001,<baud>,0  -> in: UBX+NMEA, out: NMEA, port 1 (UART1)
  snprintf(pl,sizeof(pl),"PUBX,41,1,0003,0001,%u,0",(unsigned)baud);
  sendNMEA(pl);
  delay(150);
  GPS.begin(baud, SERIAL_8N1, GPS_RX, GPS_TX);
  delay(150);
}

// Verifikasi kasar: hitung GGA per 2.5 detik
static float verifyGGA(uint32_t ms=2500){
  flushGPS();
  uint32_t t0=millis(); uint16_t gga=0; String s;
  while (millis()-t0<ms){
    int c=GPS.read(); if (c<0){ delay(1); continue; }
    char ch=(char)c;
    if (ch=='\n'){ if (s.startsWith("$GPGGA")||s.startsWith("$GNGGA")) gga++; s=""; }
    else if (ch!='\r') s+=ch;
  }
  return gga / (ms/1000.0f);
}

// Apply 20Hz (meas=50ms) + hanya GGA/RMC pada baud saat ini
static bool apply10Hz_currentBaud(){
  bool ok = sendVALSET_RAM([](uint8_t*& p){
    // 20 Hz => meas=50 ms (kalau mau 10 Hz: 100 ms)
    putKeyU2(p, K_RATE_MEAS, 50);
    putKeyU2(p, K_RATE_NAV , 1);
    // Protocol: in UBX+NMEA, out NMEA
    putKeyL1(p, K_UART1IN_UBX , 1);
    putKeyL1(p, K_UART1IN_NMEA, 1);
    putKeyL1(p, K_UART1OUT_NMEA,1);
    // NMEA message rates (per solution)
    putKeyU1(p, K_MSG_GGA_UART1, 1);
    putKeyU1(p, K_MSG_RMC_UART1, 1);
    putKeyU1(p, K_MSG_GSA_UART1, 0);
    putKeyU1(p, K_MSG_GSV_UART1, 0);
    putKeyU1(p, K_MSG_GLL_UART1, 0);
    putKeyU1(p, K_MSG_VTG_UART1, 0);
    putKeyU1(p, K_MSG_ZDA_UART1, 0);
  });
  gpsLogf("  [ACK] VALSET 20Hz: %s", ok ? "OK" : "FAIL");
  return ok;
}

// --- pindah baud ke 115200 tanpa ubah rate 10Hz ---
static bool switchTo115200(){
  bool okB = sendVALSET_RAM([](uint8_t*& p){ putKeyU4(p, K_UART1_BAUD, 115200); });
  gpsLogf("  [ACK] Set BAUD 115200 (VALSET): %s", okB?"OK":"FAIL");
  delay(120);
#if ARDUINO >= 10805
  GPS.updateBaudRate(115200);
#else
  GPS.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
#endif
  delay(150);

  if (!seeNMEA(700)) {
    gpsLogf("  [FALLBACK] $PUBX,41 -> 115200 …");
    sendPUBX41(115200);
    if (!seeNMEA(700)) {
      gpsLogf("  [ERR] NMEA belum terlihat @115200.");
      return false;
    }
  }
  gpsLogf("  [OK] NMEA aktif @115200.");
  return true;
}

// --- orkestrator: deteksi baud → (kalau bukan 115200) switch baud → set 10Hz → verifikasi ---
void gpsAutoSetup10Hz() {
  gpsLogf("[GPS] Auto-detect baud → force 20Hz @115200…");

  // 1) deteksi baud awal
  uint32_t cur=0;
  if (!scanBaud(cur)) { gpsLogf("  [ERR] NMEA tidak ditemukan. Cek wiring RX/TX & port UART modul."); return; }
  gpsLogf("  [OK] NMEA terdeteksi @ %u bps", (unsigned)cur);

  // 2) kalau bukan 115200 → pindah dulu ke 115200
  if (cur != 115200) {
    gpsLogf("  [STEP] Switch ke 115200 dulu supaya bandwidth lega…");
    if (!switchTo115200()) { gpsLogf("  [FAIL] Gagal switch ke 115200. Stop."); return; }
  } else {
#if ARDUINO >= 10805
    GPS.updateBaudRate(115200);
#else
    GPS.begin(115200, SERIAL_8N1, GPS_RX, GPS_TX);
#endif
    delay(120);
  }

  // 3) apply 20Hz + cuma GGA/RMC
  gpsLogf("  [STEP] Set 20Hz + GGA/RMC only (VALSET) @115200…");
  apply10Hz_currentBaud();
  float hz = verifyGGA(2500);
  gpsLogf("  [VERIFY] GGA ~= %.1f Hz @115200", hz);
  if (hz >= 18.0f) { gpsLogf("  [DONE] 20Hz OK @115200."); return; }

  // 4) kalau belum, enable UBX input via PUBX lalu apply ulang
  gpsLogf("  [FALLBACK] Enable UBX via $PUBX,41 lalu apply 20Hz ulang…");
  sendPUBX41(115200);
  apply10Hz_currentBaud();
  hz = verifyGGA(2500);
  gpsLogf("  [VERIFY] GGA ~= %.1f Hz @115200", hz);
  if (hz >= 18.0f) { gpsLogf("  [DONE] 20Hz OK setelah PUBX."); return; }

  gpsLogf("  [FAIL] Masih <20Hz. Modul mungkin menolak setting/port bukan UART1.");
}

/************  END AUTO BAUD + 20 Hz SETUP  ************/

void setup(){

}

void loop(){

}