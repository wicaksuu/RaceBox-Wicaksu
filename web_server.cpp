#include "web_server.h"

namespace {
WebServer server(80);
bool running = false;
std::function<String()> statusProvider;
}

void WebSrv::setStatusProvider(std::function<String()> fn) {
  statusProvider = fn;
}

void WebSrv::start(Logger& log) {
  if (running) return;
  server.on("/", [](){ server.send(200,"text/plain","OK"); });
  server.on("/status", [](){
    String payload = statusProvider ? statusProvider() : String("{}");
    server.send(200,"application/json",payload);
  });
  server.begin();
  running = true;
  log.info("WebServer started");
}

void WebSrv::stop() {
  if (!running) return;
  server.stop();
  running = false;
}

void WebSrv::tick() {
  if (running) server.handleClient();
}

