#include "logger.h"

/** @brief Initialize logger with target serial stream and buffer size. */
void Logger::init(Stream* serial, size_t max_lines) {
  _serial = serial;
  _max = max_lines;
  _lines.reserve(max_lines);
}

void Logger::info(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt); logv("INFO", fmt, ap); va_end(ap);
}

void Logger::warn(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt); logv("WARN", fmt, ap); va_end(ap);
}

void Logger::error(const char* fmt, ...) {
  va_list ap; va_start(ap, fmt); logv("ERR", fmt, ap); va_end(ap);
}

void Logger::logv(const char* level, const char* fmt, va_list ap) {
  char buf[256];
  vsnprintf(buf, sizeof(buf), fmt, ap);
  String line = String("[") + level + "] " + buf;
  if (_serial) {
    _serial->println(line);
  }
  if (_lines.size() >= _max) {
    _lines.erase(_lines.begin());
  }
  _lines.push_back(line);
  if (onLine) onLine(line);
}

