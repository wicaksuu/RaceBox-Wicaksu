#pragma once
/* Serial + TFT logger. Menampilkan welcome + live boot/system log.
   - Ring buffer supaya tidak boros RAM
   - API printf-like: logf(), logln() */
#include <Arduino.h>
#include <lvgl.h>

// Buat layar log: judul welcome & panel log di bawahnya.
// Panggil setelah display LVGL siap.
void logview_init(const char* title);

// Tambah baris log (newline otomatis)
void logln(const String& s);

// printf-style log (otomatis newline). Contoh: logf("WiFi %s", "OK");
void logf(const char* fmt, ...) __attribute__((format(printf, 1, 2)));

// Ambil isi log untuk web endpoint
String logview_get_text();

// Opsional: batasi panjang buffer (default ~6KB)
void logview_set_maxlen(size_t maxlen);

// Jadikan callback untuk lv_log_register_print_cb (akan route ke Serial + panel)
void logview_lvgl_log_cb(lv_log_level_t level, const char* buf);
