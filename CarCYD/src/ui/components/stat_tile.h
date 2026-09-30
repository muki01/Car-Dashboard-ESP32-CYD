/**
 * @file stat_tile.h
 * Compact read-out used in the dashboard corners:
 *
 *   92 °C
 *   [icon] COOLANT
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

#include "../../core/types.h"

namespace ui {

class StatTile {
 public:
  static constexpr int32_t kWidth = 66;
  static constexpr int32_t kHeight = 38;

  void create(lv_obj_t* parent, bool align_right);
  void set_signal(core::SignalId id);
  core::SignalId signal() const { return signal_; }
  void update(uint32_t now_ms);
  lv_obj_t* root() const { return root_; }

 private:
  lv_obj_t* root_ = nullptr;
  lv_obj_t* icon_ = nullptr;
  lv_obj_t* value_ = nullptr;
  lv_obj_t* unit_ = nullptr;
  lv_obj_t* caption_ = nullptr;
  core::SignalId signal_ = core::SignalId::CoolantTemp;
};

}  // namespace ui
