#pragma once
#include "global.h"

// One-call setup and cooperative loop (non-blocking)
bool app_init();   // return false jika gagal init kritikal
void app_loop();   // panggil dari loop()
