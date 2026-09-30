#include "dashboard_page.h"

#include <math.h>
#include <stdio.h>

#include "../../core/i18n.h"
#include "../../core/platform.h"
#include "../../core/signals.h"
#include "../../core/units.h"
#include "../../core/vehicle_state.h"
#include "../components/dialog.h"
#include "../signal_format.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui {
namespace {

constexpr int32_t kRingDiameter = 176;
constexpr uint32_t kSlowUpdateMs = 200;  // corner values / mini gauges

// Signal picker context (only one picker can be open).
struct PickContext {
  DashboardPage* page;
  bool corner;
  uint8_t slot;
};
PickContext g_pick{};

bool pickable_for_gauge(core::SignalId id) {
  return id != core::SignalId::RunTime && id != core::SignalId::Gear;
}

}  // namespace

const char* DashboardPage::title() const { return core::tr(core::Str::PAGE_DASHBOARD); }

void DashboardPage::create(lv_obj_t* parent) {
  root_ = box(parent);
  lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
  lv_obj_set_clickable(root_, true);
  // Gestures bubble up from the children and stop here (LVGL delivers them to
  // the first ancestor that does not bubble further).
  lv_obj_set_gesture_bubble(root_, false);
  lv_obj_add_event_cb(root_, gesture_cb, LV_EVENT_GESTURE, this);

  // ---- View 0: cluster -----------------------------------------------------------------
  views_[0] = box(root_);
  lv_obj_set_size(views_[0], lv_pct(100), lv_pct(100));
  lv_obj_set_gesture_bubble(views_[0], true);

  ring_.create(views_[0], kRingDiameter);
  lv_obj_center(ring_.root());

  const lv_align_t corners[core::kCornerSlots] = {LV_ALIGN_TOP_LEFT, LV_ALIGN_TOP_RIGHT, LV_ALIGN_BOTTOM_LEFT,
                                                  LV_ALIGN_BOTTOM_RIGHT};
  for (uint8_t i = 0; i < core::kCornerSlots; ++i) {
    const bool right = (i % 2) == 1;
    tiles_[i].create(views_[0], right);
    lv_obj_align(tiles_[i].root(), corners[i], right ? -6 : 6, i < 2 ? 6 : -6);
    lv_obj_set_gesture_bubble(tiles_[i].root(), true);
    lv_obj_add_event_cb(tiles_[i].root(), tile_long_press_cb, LV_EVENT_LONG_PRESSED,
                        reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    lv_obj_set_user_data(tiles_[i].root(), this);
  }

  // ---- View 1: engine gauges -------------------------------------------------------------
  views_[1] = box(root_);
  lv_obj_set_size(views_[1], lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_top(views_[1], 8, 0);
  lv_obj_set_style_pad_row(views_[1], 10, 0);
  lv_obj_set_flex_flow(views_[1], LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_flex_align(views_[1], LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
  lv_obj_set_gesture_bubble(views_[1], true);
  for (uint8_t i = 0; i < core::kGaugeSlots; ++i) {
    gauges_[i].create(views_[1], 86, 80);
    lv_obj_set_gesture_bubble(gauges_[i].root(), true);
    lv_obj_add_event_cb(gauges_[i].root(), gauge_long_press_cb, LV_EVENT_LONG_PRESSED,
                        reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    lv_obj_set_user_data(gauges_[i].root(), this);
  }

  // ---- Page indicator ------------------------------------------------------------------
  lv_obj_t* dots = row(root_, 8);
  lv_obj_set_ignore_layout(dots, true);
  lv_obj_align(dots, LV_ALIGN_BOTTOM_MID, 0, -4);
  for (uint8_t i = 0; i < 2; ++i) {
    dots_[i] = box(dots);
    lv_obj_set_size(dots_[i], 6, 6);
    lv_obj_set_style_radius(dots_[i], LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_opa(dots_[i], LV_OPA_COVER, 0);
    lv_obj_set_clickable(dots_[i], true);
    lv_obj_set_ext_click_area(dots_[i], 14);
    lv_obj_add_event_cb(dots_[i], dot_cb, LV_EVENT_CLICKED, this);
  }

  apply_settings();
  show_view(core::settings::get().dash_view, false);
}

void DashboardPage::apply_settings() {
  const core::Settings& s = core::settings::get();
  ring_.configure(s.rpm_max, s.redline_rpm, s.shift_rpm, s.shift_light);
  for (uint8_t i = 0; i < core::kCornerSlots; ++i) tiles_[i].set_signal(s.corner_slots[i]);
  for (uint8_t i = 0; i < core::kGaugeSlots; ++i) gauges_[i].set_signal(s.gauge_slots[i]);
  last_slow_update_ms_ = 0;
}

void DashboardPage::on_settings_changed(uint32_t changes) {
  using namespace core::settings;
  if (changes & (kChangeGauges | kChangeLayout | kChangeUnits | kChangeWarnings)) apply_settings();
}

void DashboardPage::on_show() {
  // Like a real cluster the needle sweep runs once per power cycle, not after UI rebuilds.
  static bool swept = false;
  if (!swept) {
    swept = true;
    ring_.start_sweep(platform::millis());
  }
  last_slow_update_ms_ = 0;
}

void DashboardPage::show_view(uint8_t view, bool persist) {
  view_ = view > 1 ? 0 : view;
  set_visible(views_[0], view_ == 0);
  set_visible(views_[1], view_ == 1);
  for (uint8_t i = 0; i < 2; ++i) {
    lv_obj_set_style_bg_color(dots_[i], i == view_ ? color::accent() : color::text_faint(), 0);
  }
  last_slow_update_ms_ = 0;
  if (persist && core::settings::get().dash_view != view_) {
    core::settings::edit().dash_view = view_;
    core::settings::commit(core::settings::kChangeLayout);
  }
}

void DashboardPage::update(uint32_t now_ms) {
  const bool slow_due = now_ms - last_slow_update_ms_ >= kSlowUpdateMs || last_slow_update_ms_ == 0;
  if (slow_due) last_slow_update_ms_ = now_ms;

  if (view_ == 0) {
    const core::VehicleState& v = core::vehicle();
    ring_.set_rpm(v.value_or(core::SignalId::Rpm, now_ms, 0.0f), v.fresh(core::SignalId::Rpm, now_ms));

    const SignalText speed = format_signal(core::SignalId::Speed, now_ms);
    ring_.set_speed(speed.value, speed.unit);

    if (v.fresh(core::SignalId::Gear, now_ms)) {
      char gear[4];
      core::units::format_value(core::SignalId::Gear, v.get(core::SignalId::Gear).value, core::settings::get(), gear,
                                sizeof(gear));
      ring_.set_gear(gear);
    } else {
      ring_.set_gear(nullptr);
    }
    ring_.tick(now_ms);

    if (slow_due) {
      for (auto& tile : tiles_) tile.update(now_ms);
    }
  } else if (slow_due) {
    for (auto& gauge : gauges_) gauge.update(now_ms);
  }
}

// ---- Events ---------------------------------------------------------------------------------

void DashboardPage::gesture_cb(lv_event_t* e) {
  auto* self = static_cast<DashboardPage*>(lv_event_get_user_data(e));
  const lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_active());
  if (dir == LV_DIR_LEFT && self->view_ == 0) {
    self->show_view(1, true);
  } else if (dir == LV_DIR_RIGHT && self->view_ == 1) {
    self->show_view(0, true);
  }
}

void DashboardPage::dot_cb(lv_event_t* e) {
  auto* self = static_cast<DashboardPage*>(lv_event_get_user_data(e));
  lv_obj_t* dot = lv_event_get_current_target_obj(e);
  self->show_view(dot == self->dots_[1] ? 1 : 0, true);
}

void DashboardPage::tile_long_press_cb(lv_event_t* e) {
  auto* self = static_cast<DashboardPage*>(lv_obj_get_user_data(lv_event_get_current_target_obj(e)));
  self->open_picker(true, static_cast<uint8_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}

void DashboardPage::gauge_long_press_cb(lv_event_t* e) {
  auto* self = static_cast<DashboardPage*>(lv_obj_get_user_data(lv_event_get_current_target_obj(e)));
  self->open_picker(false, static_cast<uint8_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}

void DashboardPage::open_picker(bool corner, uint8_t slot) {
  platform::play(platform::Sound::Confirm);
  g_pick = PickContext{this, corner, slot};
  const core::Settings& s = core::settings::get();
  const core::SignalId current = corner ? s.corner_slots[slot] : s.gauge_slots[slot];

  lv_obj_t* body = dialog::sheet(core::tr(core::Str::DASH_PICK_TITLE));
  lv_obj_t* selected_item = nullptr;
  for (uint8_t i = 0; i < core::kSignalCount; ++i) {
    const auto id = static_cast<core::SignalId>(i);
    if (id == core::SignalId::Speed || id == core::SignalId::Rpm) continue;  // always on the ring
    if (!corner && !pickable_for_gauge(id)) continue;

    lv_obj_t* item = card(body);
    lv_obj_set_size(item, lv_pct(100), 38);
    lv_obj_set_style_bg_color(item, color::bg(), 0);
    lv_obj_set_style_pad_hor(item, 10, 0);
    lv_obj_set_style_pad_ver(item, 0, 0);
    lv_obj_set_flex_flow(item, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(item, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(item, 10, 0);
    lv_obj_set_clickable(item, true);
    lv_obj_set_style_bg_color(item, color::card_hi(), LV_STATE_PRESSED);
    const bool selected = id == current;
    if (selected) {
      selected_item = item;
      lv_obj_set_style_border_width(item, 1, 0);
      lv_obj_set_style_border_color(item, color::accent(), 0);
    }
    icon(item, signal_icon(id), font::value(), selected ? color::accent() : color::text_dim());
    lv_obj_t* name = label(item, font::body(), color::text(), core::tr(core::signal_info(id).name));
    lv_obj_set_flex_grow(name, 1);
    if (selected) icon(item, ICON_CHECK, font::value(), color::accent());
    lv_obj_add_event_cb(item, picked_cb, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    add_touch_feedback(item);
  }
  if (selected_item != nullptr) {
    lv_obj_update_layout(body);
    lv_obj_scroll_to_view(selected_item, LV_ANIM_OFF);
  }
}

void DashboardPage::picked_cb(lv_event_t* e) {
  const auto id = static_cast<core::SignalId>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
  core::Settings& s = core::settings::edit();
  if (g_pick.corner) {
    s.corner_slots[g_pick.slot] = id;
  } else {
    s.gauge_slots[g_pick.slot] = id;
  }
  dialog::close();
  core::settings::commit(core::settings::kChangeLayout);
}

}  // namespace ui
