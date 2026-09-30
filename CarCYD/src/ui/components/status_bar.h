/**
 * @file status_bar.h
 * Top bar: page title / back navigation on the left, vehicle status icons
 * (check engine, alerts, link, demo, clock) on the right.
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace ui {

class StatusBar {
 public:
  void create(lv_obj_t* parent, lv_event_cb_t on_back, void* user);
  void set_title(const char* title);
  void show_back(bool show);
  void update(uint32_t now_ms);

 private:
  lv_obj_t* root_ = nullptr;
  lv_obj_t* back_ = nullptr;
  lv_obj_t* title_ = nullptr;
  lv_obj_t* mil_ = nullptr;
  lv_obj_t* alert_ = nullptr;
  lv_obj_t* link_ = nullptr;
  lv_obj_t* demo_ = nullptr;
  lv_obj_t* clock_ = nullptr;
  uint32_t last_update_ms_ = 0;
  bool blink_ = false;
};

}  // namespace ui
