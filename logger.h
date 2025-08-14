#pragma once
/**
 * @file logger.h
 * @brief Lightweight ring-buffer logger with optional UI hook.
 */

#include "global.h"

class Logger {
public:
  void init(Stream* serial, size_t max_lines);
  void info(const char* fmt, ...);
  void warn(const char* fmt, ...);
  void error(const char* fmt, ...);

  std::function<void(const String&)> onLine; //!< Optional callback when a line is added

private:
  void logv(const char* level, const char* fmt, va_list ap);
  Stream* _serial = nullptr;
  size_t _max = 0;
  std::vector<String> _lines; //!< Ring buffer
};

