/**
 * @file mini_gauge.h
 * Small 240 degree arc gauge with the value in the centre and a caption below.
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

#include "../../core/types.h"

namespace ui {

class MiniGauge {
 public:
  void create(lv_obj_t* parent, int32_t width, int32_t height);
  void set_signal(core::SignalId id);
  core::SignalId signal() const { return signal_; }
  void update(uint32_t now_ms);
  lv_obj_t* root() const { return root_; }

 private:
  lv_obj_t* root_ = nullptr;
  lv_obj_t* arc_ = nullptr;
  lv_obj_t* value_ = nullptr;
  lv_obj_t* unit_ = nullptr;
  lv_obj_t* caption_ = nullptr;
  core::SignalId signal_ = core::SignalId::EngineLoad;
  int32_t arc_value_ = INT32_MIN;
  core::Severity severity_ = core::Severity::Normal;
  bool severity_set_ = false;
};

}  // namespace ui
