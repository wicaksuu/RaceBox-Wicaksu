#pragma once
/**
 * @file config_manager.h
 * @brief Load and save configuration from SD card.
 */

#include "global.h"
#include "logger.h"

namespace ConfigManager {
  bool mountSD(Logger* log = nullptr);
  bool load(Config& out, Logger& log);
  bool save(const Config& in, Logger& log);
}

