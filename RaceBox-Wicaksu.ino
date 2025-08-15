/* Minimal sketch: one-call setup, non-blocking loop. */
#include "init.h"

void setup() {
  if (!app_init()) {
    // If something critical failed, keep printing to help debugging
    while (true) {
      Serial.println("Initialization FAILED — check wiring, LVGL config, or pins.");
      delay(1000);
    }
  }
}

void loop() {
  app_loop(); // no delay; fully non-blocking
  // Do your other tasks here...
}
