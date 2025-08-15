#include "init.h"

// -------------------- Local: Touch <-> LVGL bridge --------------------
static void touchscreen_read_cb(lv_indev_t* indev, lv_indev_data_t* data) {
  LV_UNUSED(indev);
  // Fast check: use IRQ when available; fallback to touched()
  if (!touchscreen.tirqTouched() && !touchscreen.touched()) {
    data->state = LV_INDEV_STATE_RELEASED;
    return;
  }

  TS_Point p = touchscreen.getPoint(); // raw coords
  // Map raw -> screen pixels
  int16_t x = map(p.x, TOUCH_X_MIN, TOUCH_X_MAX, 1, SCREEN_WIDTH);
  int16_t y = map(p.y, TOUCH_Y_MIN, TOUCH_Y_MAX, 1, SCREEN_HEIGHT);
  if (x < 0) x = 0; if (x >= SCREEN_WIDTH)  x = SCREEN_WIDTH  - 1;
  if (y < 0) y = 0; if (y >= SCREEN_HEIGHT) y = SCREEN_HEIGHT - 1;

  data->state   = LV_INDEV_STATE_PRESSED;
  data->point.x = x;
  data->point.y = y;
}

// -------------------- Local: Simple demo GUI --------------------
static int btn1_count = 0;

static void on_btn1(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_CLICKED) {
    btn1_count++;
    LV_LOG_USER("Button clicked %d", btn1_count);
  }
}

static void on_btn2(lv_event_t* e) {
  if (lv_event_get_code(e) == LV_EVENT_VALUE_CHANGED) {
    lv_obj_t* obj = (lv_obj_t*) lv_event_get_target(e);
    LV_LOG_USER("Toggled %s", lv_obj_has_state(obj, LV_STATE_CHECKED) ? "on" : "off");
  }
}

static lv_obj_t* slider_label = nullptr;

static void on_slider(lv_event_t* e) {
  lv_obj_t* slider = (lv_obj_t*) lv_event_get_target(e);
  char buf[8];
  lv_snprintf(buf, sizeof(buf), "%d%%", (int)lv_slider_get_value(slider));
  lv_label_set_text(slider_label, buf);
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
  LV_LOG_USER("Slider changed to %d%%", (int)lv_slider_get_value(slider));
}

static void build_main_gui() {
  // Title
  lv_obj_t* text_label = lv_label_create(lv_screen_active());
  lv_label_set_long_mode(text_label, LV_LABEL_LONG_WRAP);
  lv_label_set_text(text_label, "Hello, world!");
  lv_obj_set_width(text_label, 150);
  lv_obj_set_style_text_align(text_label, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align(text_label, LV_ALIGN_CENTER, 0, -90);

  // Button
  lv_obj_t* btn1 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn1, on_btn1, LV_EVENT_ALL, nullptr);
  lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -50);
  lv_obj_remove_flag(btn1, LV_OBJ_FLAG_PRESS_LOCK);
  lv_obj_t* btn_label = lv_label_create(btn1);
  lv_label_set_text(btn_label, "Button");
  lv_obj_center(btn_label);

  // Toggle
  lv_obj_t* btn2 = lv_button_create(lv_screen_active());
  lv_obj_add_event_cb(btn2, on_btn2, LV_EVENT_ALL, nullptr);
  lv_obj_align(btn2, LV_ALIGN_CENTER, 0, 10);
  lv_obj_add_flag(btn2, LV_OBJ_FLAG_CHECKABLE);
  lv_obj_set_height(btn2, LV_SIZE_CONTENT);
  btn_label = lv_label_create(btn2);
  lv_label_set_text(btn_label, "Toggle");
  lv_obj_center(btn_label);

  // Slider + label
  lv_obj_t* slider = lv_slider_create(lv_screen_active());
  lv_obj_align(slider, LV_ALIGN_CENTER, 0, 60);
  lv_obj_add_event_cb(slider, on_slider, LV_EVENT_VALUE_CHANGED, nullptr);
  lv_slider_set_range(slider, 0, 100);
  lv_obj_set_style_anim_duration(slider, 2000, 0);
  slider_label = lv_label_create(lv_screen_active());
  lv_label_set_text(slider_label, "0%");
  lv_obj_align_to(slider_label, slider, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

// -------------------- Public: init & loop --------------------
bool app_init() {
  Serial.begin(115200);
  delay(50);
  Serial.printf("LVGL Library Version: %d.%d.%d\n",
                (int)lv_version_major(), (int)lv_version_minor(), (int)lv_version_patch());

  // LVGL core
  lv_init();
  lv_log_register_print_cb(lv_log_print_cb);

  // Touch SPI + driver
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);     // Paul's lib: void return
  touchscreen.setRotation(TOUCH_ROTATION);
  pinMode(XPT2046_IRQ, INPUT);

  // Display (TFT_eSPI via helper from LVGL Arduino port)
  g_display = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, LV_DRAW_BUF, DRAW_BUF_SIZE_BYTES);
  if (!g_display) {
    Serial.println("ERR: lv_tft_espi_create failed");
    return false;
  }
  lv_display_set_rotation(g_display, DISP_ROTATION);

  // Input device (touch)
  g_touch_indev = lv_indev_create();
  lv_indev_set_type(g_touch_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(g_touch_indev, touchscreen_read_cb);

  // Build demo GUI
  build_main_gui();

  return true;
}

// Simple non-blocking scheduler for LVGL tick & handler
void app_loop() {
  static uint32_t last_tick_ms  = 0;
  static uint32_t last_task_ms  = 0;
  constexpr uint32_t TICK_MS    = 5;   // LVGL internal tick step
  constexpr uint32_t HANDLER_MS = 5;   // how often we run lv_timer_handler()

  uint32_t now = millis();

  // Advance LVGL tick in fixed steps (catch up if loop jitter occurs)
  while ((now - last_tick_ms) >= TICK_MS) {
    lv_tick_inc(TICK_MS);
    last_tick_ms += TICK_MS;
  }

  // Service LVGL timers (render, anims, input processing)
  if ((now - last_task_ms) >= HANDLER_MS) {
    lv_timer_handler();  // non-blocking
    last_task_ms += HANDLER_MS;
  }
}
