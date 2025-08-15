#pragma once
/* One-shot application init and lightweight app loop.
   Call app_init() from setup(), and app_loop() from loop().
*/
#include <Arduino.h>
#include "global.h"

bool app_init();    // returns false if any critical init fails
void app_loop();    // non-blocking periodic servicing (LVGL tick & handler)
