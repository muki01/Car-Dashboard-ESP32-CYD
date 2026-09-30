#include "ring_gauge.h"

#include <math.h>
#include <stdio.h>

#include "../../core/util.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui {
namespace {

constexpr int32_t kStartAngle = 135;  // 0 rpm at 7:30 o'clock
constexpr int32_t kSweepAngle = 270;
constexpr int32_t kArcWidth = 12;
constexpr uint32_t kSweepUpMs = 650;
constexpr uint32_t kSweepDownMs = 650;
constexpr uint32_t kShiftBlinkMs = 90;
constexpr int32_t kRpmPerArcUnit = 10;

const char* kScaleLabels[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10", nullptr};

lv_style_t g_red_major;
lv_style_t g_red_minor;
bool g_styles_ready = false;

void init_styles() {
  if (g_styles_ready) return;
  lv_style_init(&g_red_major);
  lv_style_set_line_color(&g_red_major, color::crit());
  lv_style_set_text_color(&g_red_major, color::crit());
  lv_style_init(&g_red_minor);
  lv_style_set_line_color(&g_red_minor, color::crit());
  g_styles_ready = true;
}

float ease_out(float t) { return 1.0f - (1.0f - t) * (1.0f - t); }

}  // namespace

void RingGauge::create(lv_obj_t* parent, int32_t diameter) {
  init_styles();

  root_ = box(parent);
  lv_obj_set_size(root_, diameter, diameter);

  // ---- Tachometer arc ---------------------------------------------------------------
  arc_ = lv_arc_create(root_);
  lv_obj_remove_style_all(arc_);
  lv_obj_set_size(arc_, diameter, diameter);
  lv_obj_center(arc_);
  lv_obj_set_clickable(arc_, false);
  lv_arc_set_rotation(arc_, kStartAngle);
  lv_arc_set_bg_angles(arc_, 0, kSweepAngle);
  lv_obj_set_style_arc_width(arc_, kArcWidth, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc_, color::track(), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc_, false, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc_, kArcWidth, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc_, color::accent(), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc_, false, LV_PART_INDICATOR);

  // ---- Red zone marker (thin arc on the inner edge of the track) --------------------
  const int32_t red_d = diameter - 2 * (kArcWidth + 3);
  red_zone_ = lv_arc_create(root_);
  lv_obj_remove_style_all(red_zone_);
  lv_obj_set_size(red_zone_, red_d, red_d);
  lv_obj_center(red_zone_);
  lv_obj_set_clickable(red_zone_, false);
  lv_arc_set_rotation(red_zone_, kStartAngle);
  lv_obj_set_style_arc_width(red_zone_, 3, LV_PART_MAIN);
  lv_obj_set_style_arc_color(red_zone_, color::crit(), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(red_zone_, false, LV_PART_MAIN);
  lv_obj_set_style_arc_opa(red_zone_, LV_OPA_TRANSP, LV_PART_INDICATOR);

  // ---- Tick scale ------------------------------------------------------------------------
  const int32_t scale_d = diameter - 2 * (kArcWidth + 8);
  scale_ = lv_scale_create(root_);
  lv_obj_set_size(scale_, scale_d, scale_d);
  lv_obj_center(scale_);
  lv_scale_set_mode(scale_, LV_SCALE_MODE_ROUND_INNER);
  lv_scale_set_angle_range(scale_, kSweepAngle);
  lv_scale_set_rotation(scale_, kStartAngle);
  lv_scale_set_label_show(scale_, true);
  lv_scale_set_text_src(scale_, kScaleLabels);
  lv_obj_set_style_bg_opa(scale_, LV_OPA_TRANSP, 0);
  lv_obj_set_style_arc_width(scale_, 0, LV_PART_MAIN);
  lv_obj_set_style_length(scale_, 8, LV_PART_INDICATOR);
  lv_obj_set_style_line_width(scale_, 2, LV_PART_INDICATOR);
  lv_obj_set_style_line_color(scale_, color::text_dim(), LV_PART_INDICATOR);
  lv_obj_set_style_text_color(scale_, color::text_dim(), LV_PART_INDICATOR);
  lv_obj_set_style_text_font(scale_, font::caption(), LV_PART_INDICATOR);
  lv_obj_set_style_pad_radial(scale_, 3, LV_PART_INDICATOR);
  lv_obj_set_style_length(scale_, 4, LV_PART_ITEMS);
  lv_obj_set_style_line_width(scale_, 1, LV_PART_ITEMS);
  lv_obj_set_style_line_color(scale_, color::text_faint(), LV_PART_ITEMS);
  red_section_ = lv_scale_add_section(scale_);
  lv_scale_set_section_style_indicator(scale_, red_section_, &g_red_major);
  lv_scale_set_section_style_items(scale_, red_section_, &g_red_minor);

  // ---- Centre read-outs (positions scale with the diameter) --------------------------
  speed_ = label(root_, font::speed(), color::text(), "0");
  lv_obj_align(speed_, LV_ALIGN_CENTER, 0, -4);

  unit_ = label(root_, font::caption(), color::text_dim(), "");
  lv_obj_align(unit_, LV_ALIGN_CENTER, 0, diameter * 16 / 100);

  lv_obj_t* bottom = row(root_, 6);
  lv_obj_align(bottom, LV_ALIGN_BOTTOM_MID, 0, -6);

  // Gear indicator; turns into a flashing up-shift arrow at the shift point.
  gear_box_ = box(bottom);
  lv_obj_set_size(gear_box_, 22, 22);
  lv_obj_set_style_radius(gear_box_, 5, 0);
  lv_obj_set_style_border_width(gear_box_, 1, 0);
  lv_obj_set_style_bg_opa(gear_box_, LV_OPA_COVER, 0);
  gear_ = label(gear_box_, font::title(), color::accent(), "");
  lv_obj_center(gear_);
  set_visible(gear_box_, false);

  rpm_value_ = label(bottom, font::title(), color::text(), "--");
  label(bottom, font::caption(), color::text_dim(), "rpm");

  configure(rpm_max_, redline_rpm_, shift_rpm_, shift_light_);
}

void RingGauge::configure(uint16_t rpm_max, uint16_t redline_rpm, uint16_t shift_rpm, bool shift_light) {
  rpm_max_ = rpm_max;
  redline_rpm_ = redline_rpm;
  shift_rpm_ = shift_rpm;
  shift_light_ = shift_light;

  lv_arc_set_range(arc_, 0, rpm_max_ / kRpmPerArcUnit);
  arc_units_ = -1;
  color_state_ = 0xFF;

  lv_scale_set_range(scale_, 0, rpm_max_ / 100);
  lv_scale_set_total_tick_count(scale_, rpm_max_ / 250 + 1);
  lv_scale_set_major_tick_every(scale_, 4);
  lv_scale_set_section_range(scale_, red_section_, redline_rpm_ / 100, rpm_max_ / 100);

  const int32_t red_start = kSweepAngle * redline_rpm_ / rpm_max_;
  lv_arc_set_bg_angles(red_zone_, red_start, kSweepAngle);
}

void RingGauge::set_rpm(float rpm, bool valid) {
  target_rpm_ = core::clamp(rpm, 0.0f, static_cast<float>(rpm_max_));
  valid_ = valid;
  if (!valid) target_rpm_ = 0.0f;
}

void RingGauge::set_speed(const char* value, const char* unit) {
  set_text(speed_, value);
  set_text(unit_, unit);
}

void RingGauge::set_gear(const char* gear) {
  gear_known_ = gear != nullptr;
  if (gear_known_) snprintf(gear_text_, sizeof(gear_text_), "%s", gear);
}

void RingGauge::update_gear_box(bool shift, uint32_t now_ms) {
  // 0 hidden, 1 gear, 2 shift (lit), 3 shift (dark)
  uint8_t state = gear_known_ ? 1 : 0;
  if (shift) state = ((now_ms / kShiftBlinkMs) & 1) ? 2 : 3;
  if (state == 1) set_text(gear_, gear_text_);
  if (state == gear_state_) return;
  gear_state_ = state;

  set_visible(gear_box_, state != 0);
  if (state == 1) {
    lv_obj_set_style_bg_color(gear_box_, color::bg(), 0);
    lv_obj_set_style_border_color(gear_box_, color::accent(), 0);
    set_text_color(gear_, color::accent());
  } else if (state >= 2) {
    set_text(gear_, ICON_ARROW_UP);
    lv_obj_set_style_bg_color(gear_box_, state == 2 ? color::crit() : color::bg(), 0);
    lv_obj_set_style_border_color(gear_box_, color::crit(), 0);
    set_text_color(gear_, state == 2 ? color::text() : color::crit());
  }
}

void RingGauge::start_sweep(uint32_t now_ms) {
  sweeping_ = true;
  sweep_start_ms_ = now_ms;
}

void RingGauge::apply_arc_value(float rpm) {
  const int32_t units = static_cast<int32_t>(lroundf(rpm / kRpmPerArcUnit));
  if (units != arc_units_) {
    arc_units_ = units;
    lv_arc_set_value(arc_, units);
  }
}

void RingGauge::tick(uint32_t now_ms) {
  const uint32_t dt = last_tick_ms_ ? core::elapsed(now_ms, last_tick_ms_) : 40;
  last_tick_ms_ = now_ms;

  float display_rpm;
  if (sweeping_) {
    const uint32_t t = core::elapsed(now_ms, sweep_start_ms_);
    if (t < kSweepUpMs) {
      display_rpm = rpm_max_ * ease_out(static_cast<float>(t) / kSweepUpMs);
    } else if (t < kSweepUpMs + kSweepDownMs) {
      const float k = static_cast<float>(t - kSweepUpMs) / kSweepDownMs;
      display_rpm = rpm_max_ * (1.0f - k * k);
    } else {
      sweeping_ = false;
      shown_rpm_ = 0.0f;
      display_rpm = 0.0f;
    }
  } else {
    // ~90 ms time constant: fluid but still responsive.
    shown_rpm_ = core::ema(shown_rpm_, target_rpm_, core::ema_alpha(dt, 90));
    display_rpm = shown_rpm_;
  }
  apply_arc_value(display_rpm);

  // Indicator colour: accent -> amber approaching shift point -> red (blinking) at shift.
  uint8_t state = 0;
  const bool shift = !sweeping_ && valid_ && shift_light_ && shown_rpm_ >= shift_rpm_;
  if (!sweeping_ && valid_) {
    if (shown_rpm_ >= redline_rpm_) {
      state = 3;
    } else if (shift) {
      state = ((now_ms / kShiftBlinkMs) & 1) ? 3 : 4;
    } else if (shift_light_ && shown_rpm_ >= shift_rpm_ - 600) {
      state = 2;
    }
  }
  if (state != color_state_) {
    color_state_ = state;
    lv_color_t c = color::accent();
    if (state == 2) c = color::warn();
    if (state == 3) c = color::crit();
    if (state == 4) c = color::card_hi();
    lv_obj_set_style_arc_color(arc_, c, LV_PART_INDICATOR);
  }

  update_gear_box(shift, now_ms);

  // Digital RPM, rounded to 10 rpm to keep the text calm.
  const int32_t rpm_text = valid_ ? static_cast<int32_t>(lroundf(target_rpm_ / 10.0f)) * 10 : -1;
  if (rpm_text != rpm_text_value_) {
    rpm_text_value_ = rpm_text;
    char buf[12];
    if (rpm_text < 0) {
      snprintf(buf, sizeof(buf), "--");
    } else {
      snprintf(buf, sizeof(buf), "%ld", static_cast<long>(rpm_text));
    }
    set_text(rpm_value_, buf);
  }
}

}  // namespace ui
