#include "logview.h"

// UI elemen
static lv_obj_t* s_title = nullptr;
static lv_obj_t* s_ta    = nullptr; // text area untuk log

// Buffer log
static String s_logbuf;
static size_t s_maxlen = 6 * 1024; // ~6 KB

// Helper: jaga panjang buffer
static void trim_if_needed() {
  if (s_logbuf.length() <= s_maxlen) return;
  // buang 1/3 awal agar efisien
  size_t cut = s_maxlen / 3;
  s_logbuf.remove(0, cut);
  s_logbuf = F("[trimmed]\n") + s_logbuf;
}

// Tulis ke Serial dan ke text area
static void append_and_render(const String& lineWithNL) {
  Serial.print(lineWithNL);
  s_logbuf += lineWithNL;
  trim_if_needed();

  if (s_ta) {
    lv_textarea_add_text(s_ta, lineWithNL.c_str());
    lv_textarea_set_cursor_pos(s_ta, LV_TEXTAREA_CURSOR_LAST);
    // Scroll to bottom
    lv_obj_scroll_to_y(s_ta, LV_COORD_MAX, LV_ANIM_OFF);
  }
}

void logview_set_maxlen(size_t maxlen) { s_maxlen = maxlen; }

void logview_init(const char* title) {
  // Clean screen
  lv_obj_t* scr = lv_screen_active();
  lv_obj_clean(scr);

  // Title (welcome)
  s_title = lv_label_create(scr);
  lv_label_set_text(s_title, title ? title : "Welcome");
  lv_obj_set_style_text_font(s_title, LV_FONT_DEFAULT, 0);
  lv_obj_align(s_title, LV_ALIGN_TOP_MID, 0, 6);

  // Panel log: gunakan textarea agar auto-wrap & scroll
  s_ta = lv_textarea_create(scr);
  lv_obj_set_size(s_ta, lv_pct(100), lv_pct(80));
  lv_obj_align(s_ta, LV_ALIGN_BOTTOM_MID, 0, 0);
  lv_textarea_set_placeholder_text(s_ta, "Boot log...");
  lv_textarea_set_one_line(s_ta, false);
  lv_textarea_set_password_mode(s_ta, false);
  lv_textarea_set_cursor_click_pos(s_ta, false);
  lv_obj_set_style_bg_opa(s_ta, LV_OPA_20, 0);
  lv_obj_set_style_bg_color(s_ta, lv_color_hex(0x202020), 0);
  lv_obj_set_style_text_color(s_ta, lv_color_hex(0xE0E0E0), 0);

  // Seed log buffer on screen
  if (!s_logbuf.isEmpty()) {
    lv_textarea_set_text(s_ta, s_logbuf.c_str());
    lv_textarea_set_cursor_pos(s_ta, LV_TEXTAREA_CURSOR_LAST);
  }
}

void logln(const String& s) {
  String line = s;
  if (!line.endsWith("\n")) line += "\n";
  append_and_render(line);
}

void logf(const char* fmt, ...) {
  static char buf[256];
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  String line(buf);
  if (!line.endsWith("\n")) line += "\n";
  append_and_render(line);
}

String logview_get_text() {
  return s_logbuf;
}

void logview_lvgl_log_cb(lv_log_level_t level, const char* buf) {
  // Filter: tampilkan WARNING ke atas agar tidak bising
  (void)level;
  // Jika ingin filter: if(level >= LV_LOG_LEVEL_WARN) ...
  logf("[LVGL] %s", buf);
}
