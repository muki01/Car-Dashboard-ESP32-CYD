#include "trip_page.h"

#include <stdio.h>

#include "../../core/i18n.h"
#include "../../core/platform.h"
#include "../../core/settings.h"
#include "../../core/trip.h"
#include "../../core/units.h"
#include "../components/dialog.h"
#include "../theme.h"
#include "../ui.h"
#include "../widgets.h"

namespace ui {
namespace {

constexpr uint32_t kUpdateMs = 250;

enum StatIndex : uint8_t { kDistance, kDriveTime, kAvgSpeed, kMaxSpeed, kAvgConsumption, kFuelUsed };

struct StatDef {
  const char* glyph;
  core::Str caption;
};

const StatDef kStats[] = {
    {ICON_ROAD, core::Str::TRIP_DISTANCE},    {ICON_TIMER, core::Str::TRIP_TIME},
    {ICON_SIGMA, core::Str::TRIP_AVG_SPEED},  {ICON_ARROW_UP, core::Str::TRIP_MAX_SPEED},
    {ICON_FUEL, core::Str::TRIP_AVG_CONS},    {ICON_FUEL_RATE, core::Str::TRIP_FUEL_USED},
};

void format_seconds(float s, char* out, size_t len) {
  if (s <= 0.0f) {
    snprintf(out, len, "--");
  } else {
    snprintf(out, len, "%.1f s", static_cast<double>(s));
  }
}

}  // namespace

const char* TripPage::title() const { return core::tr(core::Str::PAGE_TRIP); }

void TripPage::create(lv_obj_t* parent) {
  root_ = column(parent, layout::kGap);
  lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(root_, layout::kPad, 0);

  // ---- 2 x 3 statistic cards -------------------------------------------------------------
  lv_obj_t* grid = box(root_);
  lv_obj_set_size(grid, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_flex_flow(grid, LV_FLEX_FLOW_ROW_WRAP);
  lv_obj_set_style_pad_row(grid, layout::kGap, 0);
  lv_obj_set_style_pad_column(grid, layout::kGap, 0);

  const int32_t card_w = (layout::kContentW - 2 * layout::kPad - layout::kGap) / 2;
  for (uint8_t i = 0; i < 6; ++i) {
    // [icon] CAPTION
    // 12.3 km
    lv_obj_t* c = card(grid);
    lv_obj_set_size(c, card_w, 46);
    lv_obj_set_style_pad_hor(c, 10, 0);
    lv_obj_set_style_pad_ver(c, 4, 0);
    lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);

    lv_obj_t* head = row(c, 5);
    lv_obj_set_width(head, lv_pct(100));
    icon(head, kStats[i].glyph, font::caption(), color::accent());
    lv_obj_t* cap = label(head, font::caption(), color::text_dim(), core::tr(kStats[i].caption));
    lv_obj_set_flex_grow(cap, 1);
    single_line(cap);

    lv_obj_t* line = row(c, 3);
    lv_obj_set_flex_align(line, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
    stats_[i].value = label(line, font::value(), color::text(), "--");
    stats_[i].unit = label(line, font::caption(), color::text_dim(), "");
    lv_obj_set_style_pad_bottom(stats_[i].unit, 3, 0);
  }

  // ---- Acceleration timer + reset --------------------------------------------------------
  lv_obj_t* bottom = row(root_, layout::kGap);
  lv_obj_set_width(bottom, lv_pct(100));
  lv_obj_set_flex_grow(bottom, 1);

  lv_obj_t* accel = card(bottom);
  lv_obj_set_height(accel, lv_pct(100));
  lv_obj_set_flex_grow(accel, 1);
  lv_obj_set_style_pad_hor(accel, 8, 0);
  lv_obj_set_style_pad_ver(accel, 3, 0);
  lv_obj_set_flex_flow(accel, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(accel, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(accel, 8, 0);
  icon(accel, ICON_STOPWATCH, font::value(), color::accent());
  lv_obj_t* accel_texts = column(accel, 0);
  lv_obj_set_flex_grow(accel_texts, 1);
  lv_obj_t* accel_head = row(accel_texts, 6);
  lv_obj_set_width(accel_head, lv_pct(100));
  accel_title_ = label(accel_head, font::caption(), color::text_dim(), "");
  lv_obj_set_flex_grow(accel_title_, 1);
  accel_state_ = chip(accel_head, "", color::text_faint());
  accel_values_ = label(accel_texts, font::body(), color::text(), "");

  lv_obj_t* reset = button(bottom, ICON_RESTORE, nullptr, ButtonStyle::Secondary, reset_cb, this);
  lv_obj_set_size(reset, 48, lv_pct(100));
  lv_obj_set_style_pad_hor(reset, 0, 0);
}

void TripPage::on_show() { last_update_ms_ = 0; }

void TripPage::update(uint32_t now_ms) {
  if (last_update_ms_ != 0 && now_ms - last_update_ms_ < kUpdateMs) return;
  last_update_ms_ = now_ms;

  const core::Settings& s = core::settings::get();
  const core::TripComputer& trip = core::trip();
  const core::TripData& d = trip.data();
  char buf[24];

  core::units::format_number(core::units::distance(d.distance_km, s), 1, buf, sizeof(buf));
  set_text(stats_[kDistance].value, buf);
  set_text(stats_[kDistance].unit, core::units::distance_label(s));

  core::units::format_hours_minutes(d.drive_time_s, buf, sizeof(buf));
  set_text(stats_[kDriveTime].value, buf);
  set_text(stats_[kDriveTime].unit, "h");

  const char* speed_unit = core::units::label(core::Quantity::Speed, s);
  core::units::format_number(core::units::convert(core::Quantity::Speed, trip.average_speed_kmh(), s), 0, buf,
                             sizeof(buf));
  set_text(stats_[kAvgSpeed].value, buf);
  set_text(stats_[kAvgSpeed].unit, speed_unit);

  core::units::format_number(core::units::convert(core::Quantity::Speed, d.max_speed_kmh, s), 0, buf, sizeof(buf));
  set_text(stats_[kMaxSpeed].value, buf);
  set_text(stats_[kMaxSpeed].unit, speed_unit);

  float consumption = 0.0f;
  if (core::units::consumption(d.fuel_litres, d.distance_km, s, consumption)) {
    core::units::format_number(consumption, 1, buf, sizeof(buf));
  } else {
    snprintf(buf, sizeof(buf), "--");
  }
  set_text(stats_[kAvgConsumption].value, buf);
  set_text(stats_[kAvgConsumption].unit, core::units::consumption_label(s));

  core::units::format_number(core::units::volume(d.fuel_litres, s), 2, buf, sizeof(buf));
  set_text(stats_[kFuelUsed].value, buf);
  set_text(stats_[kFuelUsed].unit, core::units::volume_label(s));

  // Acceleration timer.
  snprintf(buf, sizeof(buf), "%s", core::units::accel_label(s));
  set_text(accel_title_, buf);

  const char* state_text = "";
  lv_color_t state_color = color::text_faint();
  switch (trip.accel_state()) {
    case core::AccelState::Idle:
      state_text = core::tr(core::Str::TRIP_ACCEL_IDLE);
      break;
    case core::AccelState::Armed:
      state_text = core::tr(core::Str::TRIP_ACCEL_ARMED);
      state_color = color::ok();
      break;
    case core::AccelState::Running:
      state_text = core::tr(core::Str::TRIP_ACCEL_RUNNING);
      state_color = color::warn();
      break;
  }
  chip_set(accel_state_, state_text, state_color);

  char last[16];
  char best[16];
  if (trip.accel_state() == core::AccelState::Running) {
    format_seconds(trip.accel_elapsed_s(now_ms), last, sizeof(last));
  } else {
    format_seconds(d.accel_last_s, last, sizeof(last));
  }
  format_seconds(d.accel_best_s, best, sizeof(best));
  char line[64];
  snprintf(line, sizeof(line), "%s %s  ·  %s %s", core::tr(core::Str::TRIP_LAST), last,
           core::tr(core::Str::TRIP_BEST), best);
  set_text(accel_values_, line);
}

void TripPage::reset_cb(lv_event_t* e) {
  dialog::confirm(core::tr(core::Str::TRIP_RESET_TITLE), core::tr(core::Str::TRIP_RESET_TEXT),
                  core::tr(core::Str::RESET), true, reset_confirmed, lv_event_get_user_data(e));
}

void TripPage::reset_confirmed(void* user) {
  core::trip().reset();
  static_cast<TripPage*>(user)->last_update_ms_ = 0;
  platform::play(platform::Sound::Confirm);
  toast(core::Severity::Normal, core::tr(core::Str::TRIP_RESET_DONE));
}

}  // namespace ui
