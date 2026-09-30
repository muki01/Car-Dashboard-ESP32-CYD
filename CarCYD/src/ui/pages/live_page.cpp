#include "live_page.h"

#include <math.h>
#include <stdio.h>

#include "../../core/i18n.h"
#include "../../core/settings.h"
#include "../../core/signals.h"
#include "../../core/units.h"
#include "../../core/vehicle_state.h"
#include "../components/setting_rows.h"
#include "../signal_format.h"
#include "../theme.h"
#include "../ui.h"
#include "../widgets.h"

namespace ui {
namespace {

constexpr uint32_t kListUpdateMs = 250;   // 4 Hz is comfortable to read
constexpr uint32_t kSampleMs = 100;       // chart: 10 Hz
constexpr uint32_t kChartPoints = 120;    // 12 s window

/** Scale used to store display values as integers in the chart. */
int32_t chart_scale(core::SignalId id) {
  const uint8_t d = core::units::decimals(id, core::settings::get());
  return d >= 2 ? 100 : (d == 1 ? 10 : 1);
}

/** Rounds a range outwards to "nice" values so the axis does not jitter. */
void nice_range(float lo, float hi, int32_t& out_lo, int32_t& out_hi) {
  float span = hi - lo;
  if (span < 1.0f) span = 1.0f;
  const float magnitude = powf(10.0f, floorf(log10f(span)));
  const float step = span / magnitude < 3.0f ? magnitude / 2.0f : magnitude;
  out_lo = static_cast<int32_t>(floorf((lo - span * 0.1f) / step) * step);
  out_hi = static_cast<int32_t>(ceilf((hi + span * 0.1f) / step) * step);
  if (out_hi <= out_lo) out_hi = out_lo + 1;
}

}  // namespace

const char* LivePage::title() const { return core::tr(core::Str::PAGE_LIVE); }

void LivePage::create(lv_obj_t* parent) {
  root_ = box(parent);
  lv_obj_set_size(root_, lv_pct(100), lv_pct(100));

  list_ = rows::list(root_);
  lv_obj_set_style_pad_row(list_, 4, 0);

  for (uint8_t i = 0; i < core::kSignalCount; ++i) {
    const auto id = static_cast<core::SignalId>(i);
    lv_obj_t* r = card(list_);
    lv_obj_set_size(r, lv_pct(100), 40);
    lv_obj_set_style_pad_ver(r, 0, 0);
    lv_obj_set_style_pad_hor(r, 10, 0);
    lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_column(r, 8, 0);
    lv_obj_set_clickable(r, true);
    lv_obj_set_style_bg_color(r, color::card_hi(), LV_STATE_PRESSED);
    lv_obj_set_user_data(r, this);
    lv_obj_add_event_cb(r, row_clicked_cb, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    add_touch_feedback(r);

    lv_obj_t* ic = icon(r, signal_icon(id), font::value(), color::accent());
    lv_obj_set_width(ic, 22);
    lv_obj_t* name = label(r, font::body(), color::text(), core::tr(core::signal_info(id).name));
    lv_obj_set_flex_grow(name, 1);
    single_line(name);

    rows_[i].value = label(r, font::value(), color::text(), kNoValue);
    lv_obj_set_style_text_align(rows_[i].value, LV_TEXT_ALIGN_RIGHT, 0);
    rows_[i].unit = label(r, font::caption(), color::text_dim(), "");
    lv_obj_set_width(rows_[i].unit, 34);
  }
}

void LivePage::on_show() { last_list_update_ms_ = 0; }

void LivePage::update(uint32_t now_ms) {
  if (detail_ != nullptr) {
    update_detail(now_ms);
  } else if (last_list_update_ms_ == 0 || now_ms - last_list_update_ms_ >= kListUpdateMs) {
    last_list_update_ms_ = now_ms;
    update_list(now_ms);
  }
}

void LivePage::update_list(uint32_t now_ms) {
  for (uint8_t i = 0; i < core::kSignalCount; ++i) {
    const SignalText t = format_signal(static_cast<core::SignalId>(i), now_ms);
    set_text(rows_[i].value, t.value);
    set_text(rows_[i].unit, t.unit);
    set_text_color(rows_[i].value, t.valid ? color::severity(t.severity) : color::text_faint());
  }
}

// ---- Detail view -----------------------------------------------------------------------

void LivePage::open_detail(core::SignalId id) {
  if (detail_ != nullptr) close_detail();
  detail_id_ = id;
  set_visible(list_, false);

  detail_ = column(root_, layout::kGap);
  lv_obj_set_size(detail_, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(detail_, layout::kPad, 0);

  // Header card: big value + statistics.
  lv_obj_t* head = card(detail_);
  lv_obj_set_size(head, lv_pct(100), 62);
  lv_obj_set_style_pad_hor(head, 12, 0);
  lv_obj_set_flex_flow(head, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(head, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(head, 6, 0);

  // Signal name above the live value.
  lv_obj_t* left = column(head, 0);
  lv_obj_set_flex_grow(left, 1);
  lv_obj_t* name = label(left, font::caption(), color::text_dim(), core::tr(core::signal_info(id).name));
  lv_obj_set_width(name, lv_pct(100));
  single_line(name);
  lv_obj_t* value_box = row(left, 4);
  lv_obj_set_flex_align(value_box, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  detail_value_ = label(value_box, font::large(), color::text(), kNoValue);
  detail_unit_ = label(value_box, font::body(), color::text_dim(), "");
  lv_obj_set_style_pad_bottom(detail_unit_, 5, 0);

  lv_obj_t* stats = lv_obj_create(head);
  lv_obj_remove_style_all(stats);
  lv_obj_set_size(stats, 116, LV_SIZE_CONTENT);
  static const int32_t cols[] = {LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_FR(1), LV_GRID_TEMPLATE_LAST};
  static const int32_t rows_tpl[] = {LV_GRID_CONTENT, LV_GRID_CONTENT, LV_GRID_TEMPLATE_LAST};
  lv_obj_set_grid_dsc_array(stats, cols, rows_tpl);
  const core::Str captions[] = {core::Str::LIVE_MIN, core::Str::LIVE_AVG, core::Str::LIVE_MAX};
  lv_obj_t** values[] = {&stat_min_, &stat_avg_, &stat_max_};
  for (uint8_t c = 0; c < 3; ++c) {
    lv_obj_t* cap = label(stats, font::caption(), color::text_faint(), core::tr(captions[c]));
    lv_obj_set_grid_cell(cap, LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, 0, 1);
    *values[c] = label(stats, font::title(), color::text_dim(), kNoValue);
    lv_obj_set_grid_cell(*values[c], LV_GRID_ALIGN_CENTER, c, 1, LV_GRID_ALIGN_CENTER, 1, 1);
  }

  // Chart card.
  lv_obj_t* chart_card = card(detail_);
  lv_obj_set_width(chart_card, lv_pct(100));
  lv_obj_set_flex_grow(chart_card, 1);
  lv_obj_set_style_pad_all(chart_card, 6, 0);

  chart_ = lv_chart_create(chart_card);
  lv_obj_set_size(chart_, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_opa(chart_, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(chart_, 0, 0);
  lv_obj_set_style_pad_all(chart_, 2, 0);
  lv_obj_set_style_line_color(chart_, color::stroke(), LV_PART_MAIN);
  lv_obj_set_style_line_width(chart_, 1, LV_PART_MAIN);
  lv_obj_set_style_line_width(chart_, 2, LV_PART_ITEMS);
  lv_obj_set_style_size(chart_, 0, 0, LV_PART_INDICATOR);
  lv_chart_set_type(chart_, LV_CHART_TYPE_LINE);
  lv_chart_set_update_mode(chart_, LV_CHART_UPDATE_MODE_SHIFT);
  lv_chart_set_point_count(chart_, kChartPoints);
  lv_chart_set_div_line_count(chart_, 4, 0);
  lv_obj_set_clickable(chart_, false);
  series_ = lv_chart_add_series(chart_, color::accent(), LV_CHART_AXIS_PRIMARY_Y);
  lv_chart_set_all_values(chart_, series_, LV_CHART_POINT_NONE);

  lv_obj_t* reset = button(chart_card, ICON_RESTORE, nullptr, ButtonStyle::Ghost, reset_cb, this);
  lv_obj_set_size(reset, 34, 30);
  lv_obj_set_style_pad_hor(reset, 0, 0);
  lv_obj_set_ignore_layout(reset, true);
  lv_obj_align(reset, LV_ALIGN_TOP_RIGHT, 0, 0);

  reset_stats();
  show_back_button(true);
}

void LivePage::close_detail() {
  if (detail_ == nullptr) return;
  lv_obj_delete(detail_);
  detail_ = nullptr;
  chart_ = nullptr;
  series_ = nullptr;
  detail_id_ = core::SignalId::Count;
  set_visible(list_, true);
  set_title(title());
  show_back_button(false);
  last_list_update_ms_ = 0;
}

bool LivePage::on_back() {
  if (detail_ == nullptr) return false;
  close_detail();
  return true;
}

int LivePage::save_state() const { return detail_ != nullptr ? static_cast<int>(detail_id_) : -1; }

void LivePage::restore_state(int state) {
  if (state >= 0 && state < core::kSignalCount) open_detail(static_cast<core::SignalId>(state));
}

void LivePage::reset_stats() {
  samples_ = 0;
  sum_ = 0.0;
  axis_min_ = axis_max_ = 0;
  last_sample_ms_ = 0;
  if (chart_ != nullptr) {
    lv_chart_set_all_values(chart_, series_, LV_CHART_POINT_NONE);
    lv_chart_refresh(chart_);
  }
  set_text(stat_min_, kNoValue);
  set_text(stat_avg_, kNoValue);
  set_text(stat_max_, kNoValue);
}

void LivePage::update_detail(uint32_t now_ms) {
  if (last_sample_ms_ != 0 && now_ms - last_sample_ms_ < kSampleMs) return;
  last_sample_ms_ = now_ms;

  const core::SignalId id = detail_id_;
  const SignalText t = format_signal(id, now_ms);
  set_text(detail_value_, t.value);
  set_text(detail_unit_, t.unit);
  set_text_color(detail_value_, t.valid ? color::severity(t.severity) : color::text_faint());

  if (!t.valid) {
    lv_chart_set_next_value(chart_, series_, LV_CHART_POINT_NONE);
    return;
  }

  // Statistics are kept in display units so min/max match what is shown.
  const core::Settings& s = core::settings::get();
  const float display = core::units::convert(core::signal_info(id).quantity, core::vehicle().get(id).value, s);
  if (samples_ == 0) {
    min_ = max_ = display;
  } else {
    min_ = fminf(min_, display);
    max_ = fmaxf(max_, display);
  }
  sum_ += display;
  samples_++;

  const uint8_t decimals = core::units::decimals(id, s);
  char buf[16];
  core::units::format_number(min_, decimals, buf, sizeof(buf));
  set_text(stat_min_, buf);
  core::units::format_number(static_cast<float>(sum_ / samples_), decimals, buf, sizeof(buf));
  set_text(stat_avg_, buf);
  core::units::format_number(max_, decimals, buf, sizeof(buf));
  set_text(stat_max_, buf);

  // Chart with an axis that only moves in "nice" steps.
  const int32_t scale = chart_scale(id);
  int32_t lo, hi;
  nice_range(min_ * scale, max_ * scale, lo, hi);
  if (lo != axis_min_ || hi != axis_max_) {
    axis_min_ = lo;
    axis_max_ = hi;
    lv_chart_set_axis_range(chart_, LV_CHART_AXIS_PRIMARY_Y, lo, hi);
  }
  lv_chart_set_next_value(chart_, series_, static_cast<int32_t>(lroundf(display * scale)));
}

void LivePage::row_clicked_cb(lv_event_t* e) {
  auto* self = static_cast<LivePage*>(lv_obj_get_user_data(lv_event_get_current_target_obj(e)));
  const auto id = static_cast<core::SignalId>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
  self->open_detail(id);
}

void LivePage::reset_cb(lv_event_t* e) { static_cast<LivePage*>(lv_event_get_user_data(e))->reset_stats(); }

}  // namespace ui
