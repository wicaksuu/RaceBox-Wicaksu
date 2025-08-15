#include "init.h"

void setup() {
  if (!app_init()) {
    while (true) {
      Serial.println("Init FAILED. Check wiring/User_Setup.h/LVGL config.");
      delay(1000);
    }
  }
}

void loop() {
  app_loop(); // non-blocking loop
}
