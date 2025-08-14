#include "gps_mgr.h"

GpsMgr gpsMgr;

bool GpsMgr::begin(const GpsCfg& cfg, Logger& log) {
    _cfg = cfg;
    _log = &log;
#if defined(ESP32)
    _ser = &Serial1;
    _ser->begin(cfg.baud, SERIAL_8N1, cfg.uart_rx, cfg.uart_tx);
#else
    _ser = &Serial1;
    _ser->begin(cfg.baud);
#endif
    sendRate(cfg.rate_hz);
    _log->info("GPS started @ %u baud, %u Hz", cfg.baud, cfg.rate_hz);
    return true;
}

void GpsMgr::sendRate(uint8_t hz) {
    uint16_t meas = 1000 / hz;
    uint8_t payload[6];
    payload[0] = meas & 0xFF;
    payload[1] = meas >> 8;
    payload[2] = 0x01;
    payload[3] = 0x00;
    payload[4] = 0x01;
    payload[5] = 0x00;
    uint8_t ckA = 0, ckB = 0;
    auto upd=[&](uint8_t b){ ckA += b; ckB += ckA; };
    _ser->write(0xB5); _ser->write(0x62);
    _ser->write(0x06); upd(0x06);
    _ser->write(0x08); upd(0x08);
    _ser->write(0x06); upd(0x06);
    _ser->write(0x00); upd(0x00);
    for (int i=0;i<6;i++){ _ser->write(payload[i]); upd(payload[i]); }
    _ser->write(ckA);
    _ser->write(ckB);
    _ser->flush();
}

void GpsMgr::tick(uint32_t now_ms) {
    while (_ser && _ser->available()) {
        char c = (char)_ser->read();
        if (c == '\n') {
            parseLine(_lineBuf, now_ms);
            _lineBuf = "";
        } else if (c != '\r') {
            _lineBuf += c;
        }
    }
}

static int getFieldInt(const String& s, int idx) {
    int start = 0;
    for (int i=0;i<idx && start!=-1;i++) {
        start = s.indexOf(',', start) + 1;
    }
    if (start<=0) return 0;
    int end = s.indexOf(',', start);
    if (end==-1) end = s.length();
    return s.substring(start, end).toInt();
}

static float getFieldFloat(const String& s, int idx) {
    int start = 0;
    for (int i=0;i<idx && start!=-1;i++) {
        start = s.indexOf(',', start) + 1;
    }
    if (start<=0) return 0;
    int end = s.indexOf(',', start);
    if (end==-1) end = s.length();
    return s.substring(start, end).toFloat();
}

void GpsMgr::parseLine(const String& line, uint32_t now_ms) {
    if (line.startsWith("$GNGGA") || line.startsWith("$GPGGA")) {
        int fix = getFieldInt(line, 6);
        _stat.hasFix = (fix > 0);
        _stat.sats = getFieldInt(line,7);
        _stat.hdop = getFieldFloat(line,8);
        _stat.acc_m = _stat.hdop * 5.0f; // crude estimate
    } else if (line.startsWith("$GNRMC") || line.startsWith("$GPRMC")) {
        String timeStr = line.substring(7,13);
        String dateStr;
        int idx = 0; int commaCount=0;
        while (idx < line.length() && commaCount < 9) {
            if (line[idx]==',') commaCount++;
            idx++;
        }
        if (commaCount==9) {
            int end = line.indexOf(',', idx);
            dateStr = line.substring(idx, end);
        }
        if (timeStr.length()>=6 && dateStr.length()==6) {
            _stat.time.tm_hour = timeStr.substring(0,2).toInt();
            _stat.time.tm_min  = timeStr.substring(2,4).toInt();
            _stat.time.tm_sec  = timeStr.substring(4,6).toInt();
            _stat.time.tm_mday = dateStr.substring(0,2).toInt();
            _stat.time.tm_mon  = dateStr.substring(2,4).toInt()-1;
            _stat.time.tm_year = dateStr.substring(4,6).toInt()+100; // years since 1900
            _stat.timeValid = true;
        }
    }
    if (_lastSentence != 0) {
        uint32_t dt = now_ms - _lastSentence;
        if (dt > 0) _stat.rate_hz = 1000.0f / dt;
    }
    _lastSentence = now_ms;
}

