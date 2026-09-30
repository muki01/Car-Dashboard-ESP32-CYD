/**
 * @file ring_gauge.h
 * Main cluster gauge: 270 degree tachometer ring with a red zone, tick scale,
 * a digital speed read-out in the centre and a gear / up-shift indicator.
 *
 * The ring value is low-pass filtered every frame, which gives needle-like
 * motion even though vehicle data typically arrives at 10-20 Hz. On first
 * show the ring performs the classic start-up "needle sweep".
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace ui {

class RingGauge {
 public:
  void create(lv_obj_t* parent, int32_t diameter);
  void configure(uint16_t rpm_max, uint16_t redline_rpm, uint16_t shift_rpm, bool shift_light);

  void set_rpm(float rpm, bool valid);
  void set_speed(const char* value, const char* unit);
  /** Gear text ("N", "3"...); nullptr hides the indicator. */
  void set_gear(const char* gear);

  /** Advances smoothing, sweep and shift-light blinking. Call every frame. */
  void tick(uint32_t now_ms);

  void start_sweep(uint32_t now_ms);

  lv_obj_t* root() const { return root_; }

 private:
  void apply_arc_value(float rpm);
  void update_gear_box(bool shift, uint32_t now_ms);

  lv_obj_t* root_ = nullptr;
  lv_obj_t* arc_ = nullptr;
  lv_obj_t* red_zone_ = nullptr;
  lv_obj_t* scale_ = nullptr;
  lv_scale_section_t* red_section_ = nullptr;
  lv_obj_t* speed_ = nullptr;
  lv_obj_t* unit_ = nullptr;
  lv_obj_t* rpm_value_ = nullptr;
  lv_obj_t* gear_box_ = nullptr;
  lv_obj_t* gear_ = nullptr;

  uint16_t rpm_max_ = 8000;
  uint16_t redline_rpm_ = 6500;
  uint16_t shift_rpm_ = 6000;
  bool shift_light_ = true;

  float target_rpm_ = 0.0f;
  float shown_rpm_ = 0.0f;
  bool valid_ = false;
  int32_t arc_units_ = -1;
  int32_t rpm_text_value_ = -1;
  uint8_t color_state_ = 0xFF;
  char gear_text_[4] = "";
  bool gear_known_ = false;
  uint8_t gear_state_ = 0xFF;

  bool sweeping_ = false;
  uint32_t sweep_start_ms_ = 0;
  uint32_t last_tick_ms_ = 0;
};

}  // namespace ui
