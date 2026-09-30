#include "diag_page.h"

#include <stdio.h>

#include "../../core/data_link.h"
#include "../../core/dtc.h"
#include "../../core/i18n.h"
#include "../../core/platform.h"
#include "../../core/util.h"
#include "../../core/vehicle_state.h"
#include "../components/dialog.h"
#include "../theme.h"
#include "../ui.h"
#include "../widgets.h"

namespace ui {
namespace {

constexpr uint32_t kSummaryUpdateMs = 1000;

lv_obj_t* square_button(lv_obj_t* parent, const char* glyph, const char* text, bool primary, lv_event_cb_t cb,
                        void* user) {
  lv_obj_t* btn = button(parent, nullptr, nullptr, primary ? ButtonStyle::Primary : ButtonStyle::Secondary, cb, user);
  lv_obj_set_size(btn, 54, 54);
  lv_obj_set_style_pad_all(btn, 0, 0);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(btn, 0, 0);
  label(btn, font::value(), primary ? color::on_accent() : color::text(), glyph);
  label(btn, font::caption(), primary ? color::on_accent() : color::text(), text);
  return btn;
}

void format_ago(uint32_t ms, char* out, size_t len) {
  const unsigned long s = ms / 1000;
  if (s < 10) {
    snprintf(out, len, "%s", core::tr(core::Str::JUST_NOW));
  } else if (s < 60) {
    snprintf(out, len, core::tr(core::Str::FMT_SECONDS_AGO), s);
  } else if (s < 3600) {
    snprintf(out, len, core::tr(core::Str::FMT_MINUTES_AGO), s / 60);
  } else {
    snprintf(out, len, core::tr(core::Str::FMT_HOURS_AGO), s / 3600);
  }
}

core::Str status_text(core::DtcStatus status) {
  switch (status) {
    case core::DtcStatus::Pending: return core::Str::DTC_PENDING;
    case core::DtcStatus::Permanent: return core::Str::DTC_PERMANENT;
    case core::DtcStatus::Stored:
    default: return core::Str::DTC_STORED;
  }
}

lv_color_t status_color(core::DtcStatus status) {
  return status == core::DtcStatus::Pending ? color::info() : color::warn();
}

void info_line(lv_obj_t* parent, core::Str key, const char* value) {
  lv_obj_t* r = row(parent, 6);
  lv_obj_set_width(r, lv_pct(100));
  lv_obj_t* k = label(r, font::body(), color::text_dim(), core::tr(key));
  lv_obj_set_flex_grow(k, 1);
  label(r, font::body(), color::text(), value);
}

}  // namespace

const char* DiagPage::title() const { return core::tr(core::Str::PAGE_DIAG); }

void DiagPage::create(lv_obj_t* parent) {
  root_ = column(parent, layout::kGap);
  lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(root_, layout::kPad, 0);

  // ---- Toolbar: status summary + actions ---------------------------------------------
  lv_obj_t* bar = row(root_, layout::kGap);
  lv_obj_set_width(bar, lv_pct(100));

  lv_obj_t* summary = card(bar);
  lv_obj_set_height(summary, 54);
  lv_obj_set_flex_grow(summary, 1);
  lv_obj_set_style_pad_hor(summary, 8, 0);
  lv_obj_set_style_pad_ver(summary, 4, 0);
  lv_obj_set_flex_flow(summary, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(summary, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(summary, 8, 0);

  mil_icon_ = icon(summary, ICON_ENGINE, font::icon24(), color::text_faint());
  lv_obj_t* texts = column(summary, 0);
  lv_obj_set_flex_grow(texts, 1);
  lv_obj_t* caption = label(texts, font::caption(), color::text_dim(), core::tr(core::Str::DIAG_MIL_CAPTION));
  lv_obj_set_width(caption, lv_pct(100));
  single_line(caption);
  mil_state_ = label(texts, font::title(), color::text(), "");
  lv_obj_set_width(mil_state_, lv_pct(100));
  single_line(mil_state_);

  scan_btn_ = square_button(bar, ICON_SCAN, core::tr(core::Str::DIAG_SCAN), true, scan_cb, this);
  clear_btn_ = square_button(bar, ICON_DELETE, core::tr(core::Str::DIAG_CLEAR), false, clear_cb, this);

  // ---- Result line: number of codes and age of the last scan -------------------------
  scan_info_ = label(root_, font::caption(), color::text_dim(), "");
  lv_obj_set_width(scan_info_, lv_pct(100));
  lv_obj_set_style_pad_left(scan_info_, 4, 0);
  single_line(scan_info_);

  // ---- Content: code list or state message --------------------------------------------
  content_ = column(root_, 4);
  lv_obj_set_width(content_, lv_pct(100));
  lv_obj_set_flex_grow(content_, 1);
  lv_obj_set_scrollable(content_, true);
  lv_obj_set_scroll_dir(content_, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(content_, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_width(content_, 3, LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_color(content_, color::text_faint(), LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(content_, LV_OPA_COVER, LV_PART_SCROLLBAR);

  shown_revision_ = core::vehicle().dtc.revision - 1;  // force first build
}

void DiagPage::on_show() {
  last_summary_ms_ = 0;
  shown_revision_ = core::vehicle().dtc.revision - 1;
}

void DiagPage::update(uint32_t now_ms) {
  const core::DtcState& dtc = core::vehicle().dtc;
  if (dtc.revision != shown_revision_) {
    shown_revision_ = dtc.revision;
    rebuild_content(now_ms);
    report_result();
  }
  if (last_summary_ms_ == 0 || now_ms - last_summary_ms_ >= kSummaryUpdateMs) {
    last_summary_ms_ = now_ms;
    update_summary(now_ms);
  }
}

void DiagPage::update_summary(uint32_t now_ms) {
  const core::VehicleState& v = core::vehicle();
  const core::DtcState& dtc = v.dtc;

  lv_color_t mil_color = color::text_faint();
  core::Str mil_text = core::Str::DIAG_MIL_UNKNOWN;
  if (dtc.mil_known) {
    mil_color = dtc.mil_on ? color::warn() : color::ok();
    mil_text = dtc.mil_on ? core::Str::DIAG_MIL_ON : core::Str::DIAG_MIL_OFF;
  }
  set_text(mil_state_, core::tr(mil_text));
  set_text_color(mil_state_, dtc.mil_known ? mil_color : color::text_dim());
  set_text_color(mil_icon_, mil_color);

  char text[112];
  if (!dtc.scanned) {
    snprintf(text, sizeof(text), "%s", core::tr(core::Str::DIAG_NEVER_SCANNED));
  } else {
    char codes[32];
    snprintf(codes, sizeof(codes),
             core::tr(dtc.count == 1 ? core::Str::DIAG_FMT_CODE_ONE : core::Str::DIAG_FMT_CODE_MANY),
             static_cast<unsigned>(dtc.count));
    char ago[24];
    format_ago(core::elapsed(now_ms, dtc.last_scan_ms), ago, sizeof(ago));
    char scan[64];
    snprintf(scan, sizeof(scan), core::tr(core::Str::DIAG_FMT_LAST_SCAN), ago);
    snprintf(text, sizeof(text), "%s · %s", codes, scan);
  }
  set_text(scan_info_, text);

  const bool busy = dtc.operation != core::DtcOperation::Idle;
  const bool connected = v.link_state == core::LinkState::Connected;
  if (busy || !connected) {
    lv_obj_add_state(scan_btn_, LV_STATE_DISABLED);
  } else {
    lv_obj_remove_state(scan_btn_, LV_STATE_DISABLED);
  }
  const bool can_clear = !busy && connected && (dtc.count > 0 || dtc.mil_on);
  if (can_clear) {
    lv_obj_remove_state(clear_btn_, LV_STATE_DISABLED);
  } else {
    lv_obj_add_state(clear_btn_, LV_STATE_DISABLED);
  }
}

void DiagPage::rebuild_content(uint32_t now_ms) {
  const core::DtcState& dtc = core::vehicle().dtc;
  lv_obj_clean(content_);
  lv_obj_scroll_to_y(content_, 0, LV_ANIM_OFF);

  if (dtc.operation == core::DtcOperation::Reading) {
    show_busy(core::tr(core::Str::DIAG_SCANNING));
  } else if (dtc.operation == core::DtcOperation::Clearing) {
    show_busy(core::tr(core::Str::DIAG_CLEARING));
  } else if (!dtc.scanned) {
    show_state(ICON_SCAN, color::text_dim(), core::tr(core::Str::DIAG_IDLE_TITLE),
               core::tr(core::Str::DIAG_IDLE_TEXT));
  } else if (dtc.count == 0) {
    show_state(ICON_CHECK_CIRCLE, color::ok(), core::tr(core::Str::DIAG_EMPTY_TITLE),
               core::tr(core::Str::DIAG_EMPTY_TEXT));
  } else {
    show_codes();
  }
  last_summary_ms_ = 0;
  update_summary(now_ms);
}

void DiagPage::show_state(const char* glyph, lv_color_t c, const char* title_text, const char* text) {
  lv_obj_t* wrap = column(content_, 2);
  lv_obj_set_size(wrap, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  icon(wrap, glyph, font::icon48(), c);
  label(wrap, font::title(), color::text(), title_text);
  lv_obj_t* body = label(wrap, font::caption(), color::text_dim(), text);
  lv_obj_set_width(body, lv_pct(90));
  lv_label_set_long_mode(body, LV_LABEL_LONG_MODE_WRAP);
  lv_obj_set_style_text_align(body, LV_TEXT_ALIGN_CENTER, 0);
}

void DiagPage::show_busy(const char* text) {
  lv_obj_t* wrap = column(content_, 10);
  lv_obj_set_size(wrap, lv_pct(100), lv_pct(100));
  lv_obj_set_flex_align(wrap, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_t* spinner = lv_spinner_create(wrap);
  lv_obj_set_size(spinner, 44, 44);
  lv_spinner_set_anim_params(spinner, 900, 240);
  lv_obj_set_style_arc_width(spinner, 4, LV_PART_MAIN);
  lv_obj_set_style_arc_color(spinner, color::track(), LV_PART_MAIN);
  lv_obj_set_style_arc_width(spinner, 4, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(spinner, color::accent(), LV_PART_INDICATOR);
  label(wrap, font::body(), color::text_dim(), text);
}

void DiagPage::show_codes() {
  const core::DtcState& dtc = core::vehicle().dtc;
  for (uint8_t i = 0; i < dtc.count; ++i) {
    const core::Dtc& d = dtc.codes[i];
    const core::Severity severity = core::dtc::severity_of(d.code);

    lv_obj_t* r = card(content_);
    lv_obj_set_size(r, lv_pct(100), 46);
    lv_obj_set_style_pad_left(r, 12, 0);
    lv_obj_set_style_pad_right(r, 8, 0);
    lv_obj_set_style_pad_ver(r, 4, 0);
    lv_obj_set_style_clip_corner(r, true, 0);
    lv_obj_set_clickable(r, true);
    lv_obj_set_style_bg_color(r, color::card_hi(), LV_STATE_PRESSED);
    lv_obj_add_event_cb(r, code_clicked_cb, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    add_touch_feedback(r);

    lv_obj_t* stripe = box(r);
    lv_obj_set_size(stripe, 4, 46);
    lv_obj_set_pos(stripe, -12, -4);
    lv_obj_set_style_bg_color(stripe, color::severity(severity), 0);
    lv_obj_set_style_bg_opa(stripe, LV_OPA_COVER, 0);

    char code[8];
    core::dtc::format(d.code, code, sizeof(code));
    lv_obj_t* code_label = label(r, font::title(), color::text(), code);
    lv_obj_align(code_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t* status = chip(r, core::tr(status_text(d.status)), status_color(d.status));
    lv_obj_align(status, LV_ALIGN_TOP_RIGHT, 0, 1);

    lv_obj_t* desc = label(r, font::caption(), color::text_dim(), core::dtc::describe(d.code));
    lv_obj_set_width(desc, lv_pct(100));
    single_line(desc);
    lv_obj_align(desc, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  }
}

void DiagPage::report_result() {
  const core::DtcState& dtc = core::vehicle().dtc;
  if (!awaiting_result_ || dtc.operation != core::DtcOperation::Idle) return;
  awaiting_result_ = false;
  switch (dtc.last_result) {
    case core::DtcResult::ReadOk:
      platform::play(platform::Sound::Confirm);
      toast(core::Severity::Normal, core::tr(core::Str::DIAG_SCAN_DONE));
      break;
    case core::DtcResult::ClearOk:
      platform::play(platform::Sound::Confirm);
      toast(core::Severity::Normal, core::tr(core::Str::DIAG_CLEARED));
      break;
    case core::DtcResult::NotConnected:
      platform::play(platform::Sound::Error);
      toast(core::Severity::Warning, core::tr(core::Str::DIAG_NOT_CONNECTED));
      break;
    case core::DtcResult::Failed:
      platform::play(platform::Sound::Error);
      toast(core::Severity::Critical, core::tr(core::Str::DIAG_FAILED));
      break;
    case core::DtcResult::None:
      break;
  }
}

// ---- Events ------------------------------------------------------------------------------

void DiagPage::scan_cb(lv_event_t* e) {
  auto* self = static_cast<DiagPage*>(lv_event_get_user_data(e));
  self->awaiting_result_ = true;
  core::link::request_dtc_read(platform::millis());
}

void DiagPage::clear_cb(lv_event_t* e) {
  dialog::confirm(core::tr(core::Str::DIAG_CLEAR_TITLE), core::tr(core::Str::DIAG_CLEAR_TEXT),
                  core::tr(core::Str::DIAG_CLEAR), true, clear_confirmed, lv_event_get_user_data(e));
}

void DiagPage::clear_confirmed(void* user) {
  auto* self = static_cast<DiagPage*>(user);
  self->awaiting_result_ = true;
  core::link::request_dtc_clear(platform::millis());
}

void DiagPage::code_clicked_cb(lv_event_t* e) {
  const auto index = static_cast<uint8_t>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
  const core::DtcState& dtc = core::vehicle().dtc;
  if (index >= dtc.count) return;
  const core::Dtc& d = dtc.codes[index];
  const core::Severity severity = core::dtc::severity_of(d.code);

  char code[8];
  core::dtc::format(d.code, code, sizeof(code));
  lv_obj_t* body = dialog::sheet(code);

  lv_obj_t* desc = label(body, font::title(), color::text(), core::dtc::describe(d.code));
  lv_obj_set_width(desc, lv_pct(100));
  lv_label_set_long_mode(desc, LV_LABEL_LONG_MODE_WRAP);

  lv_obj_t* advice = card(body);
  lv_obj_set_size(advice, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(advice, color::bg(), 0);
  lv_obj_set_flex_flow(advice, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(advice, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(advice, 8, 0);
  icon(advice, severity == core::Severity::Critical ? ICON_ALERT : ICON_INFO, font::value(),
       color::severity(severity));
  const core::Str advice_text = severity == core::Severity::Critical  ? core::Str::DTC_ADVICE_CRITICAL
                                : severity == core::Severity::Warning ? core::Str::DTC_ADVICE_WARNING
                                                                      : core::Str::DTC_ADVICE_INFO;
  lv_obj_t* advice_label = label(advice, font::caption(), color::text(), core::tr(advice_text));
  lv_obj_set_flex_grow(advice_label, 1);
  lv_label_set_long_mode(advice_label, LV_LABEL_LONG_MODE_WRAP);

  info_line(body, core::Str::DTC_SYSTEM, core::tr(core::dtc::system_name(d.code)));
  info_line(body, core::Str::DTC_TYPE,
            core::tr(core::dtc::is_generic(d.code) ? core::Str::DTC_GENERIC : core::Str::DTC_MANUFACTURER));
  info_line(body, core::Str::DTC_STATUS, core::tr(status_text(d.status)));
}

}  // namespace ui
