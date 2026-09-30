#include "status_bar.h"

#include <stdio.h>

#include "../../core/alerts.h"
#include "../../core/vehicle_state.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui {
namespace {
constexpr uint32_t kUpdatePeriodMs = 500;
}  // namespace

void StatusBar::create(lv_obj_t* parent, lv_event_cb_t on_back, void* user) {
  root_ = box(parent);
  lv_obj_set_pos(root_, layout::kRailW, 0);
  lv_obj_set_size(root_, layout::kContentW, layout::kStatusH);
  lv_obj_set_style_bg_color(root_, color::surface(), 0);
  lv_obj_set_style_bg_opa(root_, LV_OPA_COVER, 0);
  lv_obj_set_style_border_side(root_, LV_BORDER_SIDE_BOTTOM, 0);
  lv_obj_set_style_border_width(root_, 1, 0);
  lv_obj_set_style_border_color(root_, color::stroke(), 0);
  lv_obj_set_style_pad_left(root_, 10, 0);
  lv_obj_set_style_pad_right(root_, 10, 0);
  lv_obj_set_flex_flow(root_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(root_, 8, 0);

  back_ = icon_button(root_, ICON_BACK, 26, on_back, user);
  lv_obj_set_ext_click_area(back_, 10);
  set_visible(back_, false);

  title_ = label(root_, font::title(), color::text(), "");
  lv_obj_set_flex_grow(title_, 1);
  single_line(title_);

  mil_ = icon(root_, ICON_ENGINE, font::title(), color::warn());
  alert_ = icon(root_, ICON_ALERT, font::title(), color::warn());
  demo_ = chip(root_, "DEMO", color::accent());
  link_ = icon(root_, ICON_LINK_OFF, font::title(), color::text_faint());
  clock_ = label(root_, font::title(), color::text(), "");

  set_visible(mil_, false);
  set_visible(alert_, false);
  set_visible(demo_, false);
  set_visible(clock_, false);
}

void StatusBar::set_title(const char* title) { set_text(title_, title); }

void StatusBar::show_back(bool show) { set_visible(back_, show); }

void StatusBar::update(uint32_t now_ms) {
  if (now_ms - last_update_ms_ < kUpdatePeriodMs) return;
  last_update_ms_ = now_ms;
  blink_ = !blink_;

  const core::VehicleState& v = core::vehicle();

  set_visible(mil_, v.dtc.mil_known && v.dtc.mil_on);

  // The check engine light has its own icon; the triangle covers all other alerts.
  const core::Severity alert = core::alerts().highest(false);
  set_visible(alert_, alert >= core::Severity::Warning);
  if (alert >= core::Severity::Warning) {
    // Critical alerts blink to draw attention.
    const bool dim = alert == core::Severity::Critical && blink_;
    set_text_color(alert_, dim ? color::text_faint() : color::severity(alert));
  }

  switch (v.link_state) {
    case core::LinkState::Connected:
      set_text(link_, ICON_LINK);
      set_text_color(link_, color::ok());
      break;
    case core::LinkState::Connecting:
      set_text(link_, ICON_LINK);
      set_text_color(link_, blink_ ? color::warn() : color::text_faint());
      break;
    case core::LinkState::Error:
      set_text(link_, ICON_LINK_OFF);
      set_text_color(link_, color::crit());
      break;
    case core::LinkState::Disconnected:
    default:
      set_text(link_, ICON_LINK_OFF);
      set_text_color(link_, color::text_faint());
      break;
  }

  set_visible(demo_, v.simulated);

  set_visible(clock_, v.clock_valid);
  if (v.clock_valid) {
    char buf[8];
    snprintf(buf, sizeof(buf), "%02u:%02u", v.clock_minutes / 60, v.clock_minutes % 60);
    set_text(clock_, buf);
  }
}

}  // namespace ui
