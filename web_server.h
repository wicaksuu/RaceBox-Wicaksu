#pragma once
/**
 * @file web_server.h
 * @brief Simple WebServer wrapper providing basic routes.
 */

#include "global.h"
#include "logger.h"

namespace WebSrv {
  void start(Logger& log);
  void stop();
  void tick();
  void setStatusProvider(std::function<String()> fn);
}

