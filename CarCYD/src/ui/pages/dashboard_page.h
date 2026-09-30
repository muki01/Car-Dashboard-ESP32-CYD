/**
 * @file dashboard_page.h
 * Main cluster with two views, switched by swiping or tapping the page dots:
 *   0  "Cluster" : tachometer ring with digital speed + four corner values
 *   1  "Engine"  : six mini gauges
 * Long-pressing any corner value or mini gauge lets the driver pick another
 * signal; the layout is stored in the settings.
 */
#pragma once

#include "../../core/settings.h"
#include "../components/mini_gauge.h"
#include "../components/ring_gauge.h"
#include "../components/stat_tile.h"
#include "page.h"

namespace ui {

class DashboardPage final : public Page {
 public:
  void create(lv_obj_t* parent) override;
  const char* title() const override;
  void on_show() override;
  void update(uint32_t now_ms) override;
  void on_settings_changed(uint32_t changes) override;

 private:
  void show_view(uint8_t view, bool persist);
  void apply_settings();
  void open_picker(bool corner, uint8_t slot);

  static void gesture_cb(lv_event_t* e);
  static void dot_cb(lv_event_t* e);
  static void tile_long_press_cb(lv_event_t* e);
  static void gauge_long_press_cb(lv_event_t* e);
  static void picked_cb(lv_event_t* e);

  lv_obj_t* views_[2] = {};
  lv_obj_t* dots_[2] = {};
  RingGauge ring_;
  StatTile tiles_[core::kCornerSlots];
  MiniGauge gauges_[core::kGaugeSlots];
  uint8_t view_ = 0;
  uint32_t last_slow_update_ms_ = 0;
};

}  // namespace ui
