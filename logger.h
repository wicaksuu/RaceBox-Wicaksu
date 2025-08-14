#pragma once
#include "global.h"
#include <functional>
#include <vector>

/**
 * @brief Simple ring-buffer logger that outputs to Serial and UI.
 */
class Logger {
public:
    void init(Stream* serial, size_t max_lines = LOGGER_MAX_LINES);
    void info(const char* fmt, ...);
    void warn(const char* fmt, ...);
    void error(const char* fmt, ...);

    // Callback invoked for each complete line.
    std::function<void(const String&)> onLine;

    const std::vector<String>& lines() const { return _lines; }

private:
    void log(const char* level, const char* fmt, va_list ap);
    Stream* _serial{nullptr};
    size_t _max{LOGGER_MAX_LINES};
    std::vector<String> _lines; // ring buffer
};

extern Logger logger;

