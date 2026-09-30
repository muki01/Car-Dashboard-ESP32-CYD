#include "mini_gauge.h"

#include <math.h>
#include <stdio.h>

#include "../../core/i18n.h"
#include "../../core/settings.h"
#include "../../core/signals.h"
#include "../../core/vehicle_state.h"
#include "../signal_format.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui {
namespace {
constexpr int32_t kArcScale = 10;  // arc works on integers: value * 10
}  // namespace

void MiniGauge::create(lv_obj_t* parent, int32_t width, int32_t height) {
  root_ = box(parent);
  lv_obj_set_size(root_, width, height);
  lv_obj_set_clickable(root_, true);

  const int32_t d = height < width ? height : width - 8;
  arc_ = lv_arc_create(root_);
  lv_obj_remove_style_all(arc_);
  lv_obj_set_size(arc_, d, d);
  lv_obj_align(arc_, LV_ALIGN_TOP_MID, 0, 0);
  lv_obj_set_clickable(arc_, false);
  lv_obj_set_event_bubble(arc_, true);
  lv_arc_set_rotation(arc_, 150);
  lv_arc_set_bg_angles(arc_, 0, 240);
  lv_obj_set_style_arc_width(arc_, 6, LV_PART_MAIN);
  lv_obj_set_style_arc_color(arc_, color::track(), LV_PART_MAIN);
  lv_obj_set_style_arc_rounded(arc_, true, LV_PART_MAIN);
  lv_obj_set_style_arc_width(arc_, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(arc_, color::accent(), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(arc_, true, LV_PART_INDICATOR);

  value_ = label(arc_, font::value(), color::text(), kNoValue);
  lv_obj_align(value_, LV_ALIGN_CENTER, 0, -6);
  unit_ = label(arc_, font::caption(), color::text_dim(), "");
  lv_obj_align(unit_, LV_ALIGN_CENTER, 0, 12);

  // The caption sits in the open bottom segment of the arc, so it reads as part of the gauge.
  caption_ = label(arc_, font::caption(), color::text_dim(), "");
  lv_obj_align(caption_, LV_ALIGN_BOTTOM_MID, 0, 0);
}

void MiniGauge::set_signal(core::SignalId id) {
  signal_ = id;
  float lo, hi;
  core::signal_range(id, core::settings::get(), lo, hi);
  lv_arc_set_range(arc_, static_cast<int32_t>(lo * kArcScale), static_cast<int32_t>(hi * kArcScale));
  arc_value_ = INT32_MIN;
  severity_set_ = false;

  char caption[32];
  snprintf(caption, sizeof(caption), "%s %s", signal_icon(id), core::tr(core::signal_info(id).short_name));
  set_text(caption_, caption);
}

void MiniGauge::update(uint32_t now_ms) {
  const SignalText t = format_signal(signal_, now_ms);
  set_text(value_, t.value);
  set_text(unit_, t.unit);
  set_text_color(value_, t.valid ? color::severity(t.severity) : color::text_faint());

  const core::VehicleState& v = core::vehicle();
  const int32_t value = t.valid ? static_cast<int32_t>(lroundf(v.get(signal_).value * kArcScale))
                                : lv_arc_get_min_value(arc_);
  if (value != arc_value_) {
    arc_value_ = value;
    lv_arc_set_value(arc_, value);
  }
  if (!severity_set_ || t.severity != severity_) {
    severity_ = t.severity;
    severity_set_ = true;
    lv_obj_set_style_arc_color(arc_, color::severity_accent(t.severity), LV_PART_INDICATOR);
  }
}

}  // namespace ui
