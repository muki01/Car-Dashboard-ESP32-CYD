#include "settings_page.h"

#include <stdio.h>

#include "../../config/app_config.h"
#include "../../core/i18n.h"
#include "../../core/platform.h"
#include "../../core/settings.h"
#include "../../core/units.h"
#include "../../core/vehicle_state.h"
#include "../components/dialog.h"
#include "../components/setting_rows.h"
#include "../theme.h"
#include "../ui.h"
#include "../widgets.h"

namespace ui {
namespace {

using core::settings::commit;
using core::settings::edit;
using core::settings::get;
namespace chg = core::settings;
using Section = SettingsPage::Section;

constexpr uint32_t kLiveUpdateMs = 1000;
constexpr uint16_t kRpmRangeBase = 5000;

struct CategoryDef {
  const char* glyph;
  core::Str title;
};

const CategoryDef kCategories[] = {
    {ICON_BRIGHTNESS, core::Str::SET_DISPLAY}, {ICON_RULER, core::Str::SET_UNITS},
    {ICON_GAUGE, core::Str::SET_GAUGES},       {ICON_BELL, core::Str::SET_WARNINGS},
    {ICON_VOLUME, core::Str::SET_SOUND},       {ICON_LINK, core::Str::SET_CONNECTION},
    {ICON_CHIP, core::Str::SET_SYSTEM},        {ICON_INFO, core::Str::SET_ABOUT},
};

const char* const kSpeedUnits[] = {"km/h", "mph", nullptr};
const char* const kTempUnits[] = {"°C", "°F", nullptr};
const char* const kPressureUnits[] = {"kPa", "bar", "psi", nullptr};
const char* const kRpmRanges[] = {"5000", "6000", "7000", "8000", "9000", nullptr};
const char* const kLanguages[] = {"English", "Türkçe", nullptr};
const char* g_sources[3] = {};  // translated at build time

SettingsPage* g_self = nullptr;

bool switch_on(lv_event_t* e) { return lv_obj_has_state(lv_event_get_target_obj(e), LV_STATE_CHECKED); }
int32_t slider_value(lv_event_t* e) { return lv_slider_get_value(lv_event_get_target_obj(e)); }
uint32_t selected_segment(lv_event_t* e) { return lv_buttonmatrix_get_selected_button(lv_event_get_target_obj(e)); }

void percent_text(lv_obj_t* label, int32_t value) {
  char buf[8];
  snprintf(buf, sizeof(buf), "%ld%%", static_cast<long>(value));
  set_text(label, buf);
}

// ---- Stepper formatters -------------------------------------------------------------------

void fmt_rpm(int32_t v, char* out, uint32_t len) { snprintf(out, len, "%ld rpm", static_cast<long>(v)); }

void fmt_temperature(int32_t v, char* out, uint32_t len) {
  const core::Settings& s = get();
  const float t = core::units::convert(core::Quantity::Temperature, static_cast<float>(v), s);
  snprintf(out, len, "%.0f %s", static_cast<double>(t), core::units::label(core::Quantity::Temperature, s));
}

void fmt_decivolts(int32_t v, char* out, uint32_t len) {
  snprintf(out, len, "%ld.%ld V", static_cast<long>(v / 10), static_cast<long>(v % 10));
}

void fmt_percent_or_off(int32_t v, char* out, uint32_t len) {
  if (v == 0) {
    snprintf(out, len, "%s", core::tr(core::Str::OFF));
  } else {
    snprintf(out, len, "%ld %%", static_cast<long>(v));
  }
}

/** Speed limit stepper: the lowest position (kOverspeedOff) means "off". */
constexpr int32_t kOverspeedOff = 20;

void fmt_speed_or_off(int32_t v, char* out, uint32_t len) {
  if (v <= kOverspeedOff) {
    snprintf(out, len, "%s", core::tr(core::Str::OFF));
    return;
  }
  const core::Settings& s = get();
  const float speed = core::units::convert(core::Quantity::Speed, static_cast<float>(v), s);
  snprintf(out, len, "%.0f %s", static_cast<double>(speed), core::units::label(core::Quantity::Speed, s));
}

// ---- Change handlers -----------------------------------------------------------------------

void on_auto_brightness(lv_event_t* e) {
  edit().auto_brightness = switch_on(e);
  commit(chg::kChangeDisplay);
}
void on_brightness(lv_event_t* e) {
  edit().brightness = static_cast<uint8_t>(slider_value(e));
  commit(chg::kChangeDisplay);
}
void on_night_brightness(lv_event_t* e) {
  edit().night_brightness = static_cast<uint8_t>(slider_value(e));
  commit(chg::kChangeDisplay);
}
void on_flip(lv_event_t* e) {
  edit().flip_screen = switch_on(e);
  commit(chg::kChangeOrientation);
}
void on_accent(lv_event_t* e) {
  const auto accent = static_cast<core::Accent>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e)));
  if (get().accent == accent) return;
  edit().accent = accent;
  commit(chg::kChangeTheme);
}
void on_speed_unit(lv_event_t* e) {
  edit().speed_unit = static_cast<core::SpeedUnit>(selected_segment(e));
  commit(chg::kChangeUnits);
}
void on_temp_unit(lv_event_t* e) {
  edit().temp_unit = static_cast<core::TempUnit>(selected_segment(e));
  commit(chg::kChangeUnits);
}
void on_pressure_unit(lv_event_t* e) {
  edit().pressure_unit = static_cast<core::PressureUnit>(selected_segment(e));
  commit(chg::kChangeUnits);
}
void on_rpm_range(lv_event_t* e) {
  edit().rpm_max = static_cast<uint16_t>(kRpmRangeBase + selected_segment(e) * 1000);
  commit(chg::kChangeGauges);
}
void on_redline(int32_t v, void*) {
  edit().redline_rpm = static_cast<uint16_t>(v);
  commit(chg::kChangeGauges);
}
void on_shift_light(lv_event_t* e) {
  edit().shift_light = switch_on(e);
  commit(chg::kChangeGauges);
}
void on_shift_rpm(int32_t v, void*) {
  edit().shift_rpm = static_cast<uint16_t>(v);
  commit(chg::kChangeGauges);
}
void on_reset_layout(lv_event_t*) {
  const core::Settings d = core::settings::defaults();
  core::Settings& s = edit();
  for (uint8_t i = 0; i < core::kCornerSlots; ++i) s.corner_slots[i] = d.corner_slots[i];
  for (uint8_t i = 0; i < core::kGaugeSlots; ++i) s.gauge_slots[i] = d.gauge_slots[i];
  s.dash_view = 0;
  commit(chg::kChangeLayout);
  platform::play(platform::Sound::Confirm);
  toast(core::Severity::Normal, core::tr(core::Str::SET_LAYOUT_DONE));
}
void on_coolant_warn(int32_t v, void*) {
  edit().coolant_warn_c = static_cast<uint8_t>(v);
  commit(chg::kChangeWarnings);
}
void on_voltage_warn(int32_t v, void*) {
  edit().low_voltage_dv = static_cast<uint8_t>(v);
  commit(chg::kChangeWarnings);
}
void on_fuel_warn(int32_t v, void*) {
  edit().fuel_warn_pct = static_cast<uint8_t>(v);
  commit(chg::kChangeWarnings);
}
void on_overspeed(int32_t v, void*) {
  edit().overspeed_kmh = static_cast<uint16_t>(v <= kOverspeedOff ? 0 : v);
  commit(chg::kChangeWarnings);
}
void on_touch_sound(lv_event_t* e) {
  edit().touch_sound = switch_on(e);
  commit(chg::kChangeSound);
}
void on_alert_sound(lv_event_t* e) {
  edit().alert_sound = switch_on(e);
  commit(chg::kChangeSound);
}
void on_volume(lv_event_t* e) {
  edit().volume = static_cast<uint8_t>(slider_value(e));
  commit(chg::kChangeSound);
}
void on_volume_released(lv_event_t*) { platform::play(platform::Sound::Confirm); }
void on_status_led(lv_event_t* e) {
  edit().status_led = switch_on(e);
  commit(chg::kChangeLed);
}
void on_data_source(lv_event_t* e) {
  edit().demo_mode = selected_segment(e) == 0;
  commit(chg::kChangeDataSource);
}
void on_language(lv_event_t* e) {
  const auto lang = static_cast<core::Language>(selected_segment(e));
  if (get().language == lang) return;
  edit().language = lang;
  commit(chg::kChangeLanguage);
}
void on_calibrate(lv_event_t*) { open_touch_calibration(); }
void restart_confirmed(void*) { platform::restart(); }
void on_restart(lv_event_t*) {
  dialog::confirm(core::tr(core::Str::SET_RESTART), core::tr(core::Str::SET_RESTART_TEXT),
                  core::tr(core::Str::OK), false, restart_confirmed, nullptr);
}
void factory_reset_confirmed(void*) { platform::factory_reset(); }
void on_factory_reset(lv_event_t*) {
  dialog::confirm(core::tr(core::Str::SET_FACTORY_RESET), core::tr(core::Str::SET_FACTORY_TEXT),
                  core::tr(core::Str::RESET), true, factory_reset_confirmed, nullptr);
}

void reopen_async(void* section) {
  if (g_self != nullptr) g_self->restore_state(static_cast<int>(reinterpret_cast<intptr_t>(section)));
}

const char* link_state_text(core::LinkState state) {
  switch (state) {
    case core::LinkState::Connected: return core::tr(core::Str::LINK_CONNECTED);
    case core::LinkState::Connecting: return core::tr(core::Str::LINK_CONNECTING);
    case core::LinkState::Error: return core::tr(core::Str::LINK_ERROR);
    case core::LinkState::Disconnected:
    default: return core::tr(core::Str::LINK_DISCONNECTED);
  }
}

}  // namespace

const char* SettingsPage::title() const { return core::tr(core::Str::PAGE_SETTINGS); }

void SettingsPage::create(lv_obj_t* parent) {
  g_self = this;
  root_ = box(parent);
  lv_obj_set_size(root_, lv_pct(100), lv_pct(100));
  build_master();
}

void SettingsPage::build_master() {
  master_ = rows::list(root_);
  for (int i = 0; i < static_cast<int>(Section::Count); ++i) {
    summaries_[i] = rows::nav(master_, kCategories[i].glyph, core::tr(kCategories[i].title), "", category_cb,
                              reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    lv_obj_set_user_data(lv_obj_get_parent(summaries_[i]), this);
  }
  refresh_summaries();
}

void SettingsPage::refresh_summaries() {
  const core::Settings& s = get();
  char buf[48];

  if (s.auto_brightness) {
    snprintf(buf, sizeof(buf), "%s · %u%%", core::tr(core::Str::AUTO), s.brightness);
  } else {
    snprintf(buf, sizeof(buf), "%u%%", s.brightness);
  }
  set_text(summaries_[static_cast<int>(Section::Display)], buf);

  snprintf(buf, sizeof(buf), "%s · %s · %s", core::units::label(core::Quantity::Speed, s),
           core::units::label(core::Quantity::Temperature, s), core::units::label(core::Quantity::Pressure, s));
  set_text(summaries_[static_cast<int>(Section::Units)], buf);

  snprintf(buf, sizeof(buf), "%u rpm", s.rpm_max);
  set_text(summaries_[static_cast<int>(Section::Gauges)], buf);

  char temp[16];
  fmt_temperature(s.coolant_warn_c, temp, sizeof(temp));
  snprintf(buf, sizeof(buf), "%s · %u.%u V", temp, s.low_voltage_dv / 10, s.low_voltage_dv % 10);
  set_text(summaries_[static_cast<int>(Section::Warnings)], buf);

  if (s.touch_sound || s.alert_sound) {
    snprintf(buf, sizeof(buf), "%u%%", s.volume);
  } else {
    snprintf(buf, sizeof(buf), "%s", core::tr(core::Str::OFF));
  }
  set_text(summaries_[static_cast<int>(Section::Sound)], buf);

  set_text(summaries_[static_cast<int>(Section::Connection)],
           core::tr(s.demo_mode ? core::Str::SET_SRC_DEMO : core::Str::SET_SRC_LINK));
  set_text(summaries_[static_cast<int>(Section::System)], core::i18n::language_name(s.language));
  set_text(summaries_[static_cast<int>(Section::About)], "v" APP_VERSION);
}

void SettingsPage::on_show() {
  refresh_summaries();
  last_live_ms_ = 0;
}

void SettingsPage::on_settings_changed(uint32_t changes) {
  refresh_summaries();

  if (section_ == Section::Display && night_row_ != nullptr) {
    const core::Settings& s = get();
    set_visible(night_row_, s.auto_brightness);
    set_text(day_title_, core::tr(s.auto_brightness ? core::Str::SET_DAY_BRIGHTNESS : core::Str::SET_BRIGHTNESS));
    percent_text(brightness_value_, s.brightness);
    percent_text(night_value_, s.night_brightness);
  }
  if (section_ == Section::Sound && volume_value_ != nullptr) percent_text(volume_value_, get().volume);

  // The redline / shift steppers are bounded by the tachometer range: rebuild the
  // section (deferred, the change originates from a widget inside it).
  if (section_ == Section::Gauges && (changes & chg::kChangeGauges) && get().rpm_max != built_rpm_max_) {
    built_rpm_max_ = get().rpm_max;
    lv_async_call(reopen_async, reinterpret_cast<void*>(static_cast<intptr_t>(section_)));
  }
}

SettingsPage::~SettingsPage() {
  lv_async_call_cancel(reopen_async, reinterpret_cast<void*>(static_cast<intptr_t>(Section::Gauges)));
  if (g_self == this) g_self = nullptr;
}

void SettingsPage::category_cb(lv_event_t* e) {
  auto* self = static_cast<SettingsPage*>(lv_obj_get_user_data(lv_event_get_current_target_obj(e)));
  self->open(static_cast<Section>(reinterpret_cast<intptr_t>(lv_event_get_user_data(e))));
}

void SettingsPage::restore_state(int state) {
  if (state >= 0 && state < static_cast<int>(Section::Count)) {
    // Keep the scroll position when a section is rebuilt in place.
    int32_t scroll = 0;
    if (detail_ != nullptr && section_ == static_cast<Section>(state)) scroll = lv_obj_get_scroll_y(detail_);
    open(static_cast<Section>(state));
    if (scroll > 0) {
      lv_obj_update_layout(detail_);
      lv_obj_scroll_to_y(detail_, scroll, LV_ANIM_OFF);
    }
  }
}

void SettingsPage::open(Section section) {
  close_section();
  section_ = section;
  set_visible(master_, false);

  detail_ = rows::list(root_);
  switch (section) {
    case Section::Display: build_display(detail_); break;
    case Section::Units: build_units(detail_); break;
    case Section::Gauges: build_gauges(detail_); break;
    case Section::Warnings: build_warnings(detail_); break;
    case Section::Sound: build_sound(detail_); break;
    case Section::Connection: build_connection(detail_); break;
    case Section::System: build_system(detail_); break;
    case Section::About: build_about(detail_); break;
    default: break;
  }
  set_title(core::tr(kCategories[static_cast<int>(section)].title));
  show_back_button(true);
  last_live_ms_ = 0;
}

void SettingsPage::close_section() {
  if (detail_ != nullptr) {
    lv_obj_delete(detail_);
    detail_ = nullptr;
  }
  ambient_label_ = night_row_ = day_title_ = brightness_value_ = night_value_ = nullptr;
  volume_value_ = link_status_ = memory_label_ = uptime_label_ = nullptr;
  for (auto& swatch : accent_swatches_) swatch = nullptr;
  section_ = Section::None;
}

bool SettingsPage::on_back() {
  if (detail_ == nullptr) return false;
  close_section();
  set_visible(master_, true);
  refresh_summaries();
  set_title(title());
  show_back_button(false);
  return true;
}

void SettingsPage::update(uint32_t now_ms) {
  if (detail_ == nullptr) return;
  if (last_live_ms_ != 0 && now_ms - last_live_ms_ < kLiveUpdateMs) return;
  last_live_ms_ = now_ms;
  update_live_values(now_ms);
}

void SettingsPage::update_live_values(uint32_t now_ms) {
  char buf[40];
  if (ambient_label_ != nullptr) {
    snprintf(buf, sizeof(buf), "%u%% · %u", platform::ambient_level(), platform::light_sensor_raw());
    set_text(ambient_label_, buf);
  }
  if (link_status_ != nullptr) {
    const core::LinkState state = core::vehicle().link_state;
    set_text(link_status_, link_state_text(state));
    set_text_color(link_status_, state == core::LinkState::Connected ? color::ok() : color::text_dim());
  }
  if (memory_label_ != nullptr) {
    platform::SystemInfo info;
    platform::system_info(info);
    snprintf(buf, sizeof(buf), "%lu KB (min %lu KB)", static_cast<unsigned long>(info.heap_free / 1024),
             static_cast<unsigned long>(info.heap_min_free / 1024));
    set_text(memory_label_, buf);
  }
  if (uptime_label_ != nullptr) {
    core::units::format_duration(now_ms / 1000, buf, sizeof(buf));
    set_text(uptime_label_, buf);
  }
}

// ---- Sections --------------------------------------------------------------------------------

void SettingsPage::build_display(lv_obj_t* list) {
  const core::Settings& s = get();

  rows::toggle(list, ICON_BRIGHTNESS_AUTO, core::tr(core::Str::SET_AUTO_BRIGHTNESS),
               core::tr(core::Str::SET_AUTO_BRIGHTNESS_SUB), s.auto_brightness, on_auto_brightness, nullptr);

  rows::Slider day = rows::slider(list, ICON_SUN, core::tr(core::Str::SET_BRIGHTNESS), 5, 100, s.brightness,
                                  on_brightness, nullptr);
  day_title_ = day.title;
  set_text(day_title_, core::tr(s.auto_brightness ? core::Str::SET_DAY_BRIGHTNESS : core::Str::SET_BRIGHTNESS));
  brightness_value_ = day.value;
  percent_text(brightness_value_, s.brightness);

  rows::Slider night = rows::slider(list, ICON_NIGHT, core::tr(core::Str::SET_NIGHT_BRIGHTNESS), 5, 100,
                                    s.night_brightness, on_night_brightness, nullptr);
  night_row_ = night.row;
  night_value_ = night.value;
  percent_text(night_value_, s.night_brightness);
  set_visible(night_row_, s.auto_brightness);

  ambient_label_ = rows::info(list, ICON_BRIGHTNESS, core::tr(core::Str::SET_AMBIENT), "--");

  rows::toggle(list, ICON_ROTATE, core::tr(core::Str::SET_FLIP), nullptr, s.flip_screen, on_flip, nullptr);

  // Accent colour swatches.
  lv_obj_t* r = card(list);
  lv_obj_set_size(r, lv_pct(100), LV_SIZE_CONTENT);
  lv_obj_set_style_pad_hor(r, 10, 0);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(r, 8, 0);
  lv_obj_t* header = row(r, 10);
  lv_obj_t* ic = icon(header, ICON_PALETTE, font::value(), color::accent());
  lv_obj_set_width(ic, 22);
  label(header, font::body(), color::text(), core::tr(core::Str::SET_ACCENT));
  lv_obj_t* swatches = row(r, 0);
  lv_obj_set_width(swatches, lv_pct(100));
  lv_obj_set_flex_align(swatches, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  for (uint8_t i = 0; i < static_cast<uint8_t>(core::Accent::Count); ++i) {
    const auto accent = static_cast<core::Accent>(i);
    lv_obj_t* sw = box(swatches);
    lv_obj_set_size(sw, 28, 28);
    lv_obj_set_style_radius(sw, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(sw, color::accent_of(accent), 0);
    lv_obj_set_style_bg_opa(sw, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(sw, color::text(), 0);
    lv_obj_set_style_border_width(sw, s.accent == accent ? 3 : 0, 0);
    lv_obj_set_clickable(sw, true);
    lv_obj_set_ext_click_area(sw, 8);
    lv_obj_add_event_cb(sw, on_accent, LV_EVENT_CLICKED, reinterpret_cast<void*>(static_cast<intptr_t>(i)));
    add_touch_feedback(sw);
    accent_swatches_[i] = sw;
  }
}

void SettingsPage::build_units(lv_obj_t* list) {
  const core::Settings& s = get();
  rows::segmented(list, ICON_SPEED, core::tr(core::Str::SET_UNIT_SPEED), kSpeedUnits,
                  static_cast<uint32_t>(s.speed_unit), on_speed_unit, nullptr);
  rows::segmented(list, ICON_THERMOMETER, core::tr(core::Str::SET_UNIT_TEMP), kTempUnits,
                  static_cast<uint32_t>(s.temp_unit), on_temp_unit, nullptr);
  rows::segmented(list, ICON_TURBO, core::tr(core::Str::SET_UNIT_PRESSURE), kPressureUnits,
                  static_cast<uint32_t>(s.pressure_unit), on_pressure_unit, nullptr);
}

void SettingsPage::build_gauges(lv_obj_t* list) {
  const core::Settings& s = get();
  built_rpm_max_ = s.rpm_max;
  const uint32_t range_index = s.rpm_max >= kRpmRangeBase ? (s.rpm_max - kRpmRangeBase) / 1000 : 0;
  rows::segmented(list, ICON_GAUGE, core::tr(core::Str::SET_RPM_MAX), kRpmRanges, range_index > 4 ? 4 : range_index,
                  on_rpm_range, nullptr);
  rows::stepper(list, ICON_ALERT, core::tr(core::Str::SET_REDLINE), 2000, s.rpm_max, 250, s.redline_rpm, fmt_rpm,
                on_redline, nullptr);
  rows::toggle(list, ICON_LED, core::tr(core::Str::SET_SHIFT_LIGHT), nullptr, s.shift_light, on_shift_light,
               nullptr);
  rows::stepper(list, ICON_SHIFT, core::tr(core::Str::SET_SHIFT_RPM), 1500, s.rpm_max, 100, s.shift_rpm, fmt_rpm,
                on_shift_rpm, nullptr);
  rows::action(list, ICON_VIEW_DASH, core::tr(core::Str::SET_LAYOUT_RESET), color::text(), on_reset_layout, nullptr);
  rows::note(list, core::tr(core::Str::SET_LAYOUT_HINT));
}

void SettingsPage::build_warnings(lv_obj_t* list) {
  const core::Settings& s = get();
  rows::stepper(list, ICON_COOLANT, core::tr(core::Str::SET_COOLANT_WARN), 90, 125, 1, s.coolant_warn_c,
                fmt_temperature, on_coolant_warn, nullptr);
  rows::stepper(list, ICON_BATTERY, core::tr(core::Str::SET_VOLTAGE_WARN), 105, 125, 1, s.low_voltage_dv,
                fmt_decivolts, on_voltage_warn, nullptr);
  rows::stepper(list, ICON_FUEL, core::tr(core::Str::SET_FUEL_WARN), 0, 30, 1, s.fuel_warn_pct, fmt_percent_or_off,
                on_fuel_warn, nullptr);
  rows::stepper(list, ICON_SPEED_LIMIT, core::tr(core::Str::SET_OVERSPEED), kOverspeedOff, 250, 10,
                s.overspeed_kmh == 0 ? kOverspeedOff : s.overspeed_kmh, fmt_speed_or_off, on_overspeed, nullptr);
}

void SettingsPage::build_sound(lv_obj_t* list) {
  const core::Settings& s = get();
  rows::toggle(list, ICON_TOUCH, core::tr(core::Str::SET_TOUCH_SOUND), nullptr, s.touch_sound, on_touch_sound,
               nullptr);
  rows::toggle(list, ICON_BELL, core::tr(core::Str::SET_ALERT_SOUND), nullptr, s.alert_sound, on_alert_sound,
               nullptr);
  rows::Slider volume =
      rows::slider(list, ICON_VOLUME, core::tr(core::Str::SET_VOLUME), 0, 100, s.volume, on_volume, nullptr);
  lv_obj_add_event_cb(volume.slider, on_volume_released, LV_EVENT_RELEASED, nullptr);
  volume_value_ = volume.value;
  percent_text(volume_value_, s.volume);
  rows::toggle(list, ICON_LED, core::tr(core::Str::SET_STATUS_LED), core::tr(core::Str::SET_STATUS_LED_SUB),
               s.status_led, on_status_led, nullptr);
}

void SettingsPage::build_connection(lv_obj_t* list) {
  g_sources[0] = core::tr(core::Str::SET_SRC_DEMO);
  g_sources[1] = core::tr(core::Str::SET_SRC_LINK);
  g_sources[2] = nullptr;
  rows::segmented(list, ICON_DATABASE, core::tr(core::Str::SET_DATA_SOURCE), g_sources, get().demo_mode ? 0 : 1,
                  on_data_source, nullptr);
  link_status_ = rows::info(list, ICON_LINK, core::tr(core::Str::SET_LINK_STATUS), "");
  rows::note(list, core::tr(core::Str::SET_DEMO_HINT));
}

void SettingsPage::build_system(lv_obj_t* list) {
  rows::segmented(list, ICON_LANGUAGE, core::tr(core::Str::SET_LANGUAGE), kLanguages,
                  static_cast<uint32_t>(get().language), on_language, nullptr);
  rows::action(list, ICON_TARGET, core::tr(core::Str::SET_CALIBRATE), color::text(), on_calibrate, nullptr);
  rows::action(list, ICON_RESTART, core::tr(core::Str::SET_RESTART), color::text(), on_restart, nullptr);
  rows::action(list, ICON_RESTORE, core::tr(core::Str::SET_FACTORY_RESET), color::crit(), on_factory_reset, nullptr);
}

void SettingsPage::build_about(lv_obj_t* list) {
  platform::SystemInfo info;
  platform::system_info(info);
  char buf[48];

  rows::info(list, ICON_CAR_INFO, core::tr(core::Str::ABOUT_FIRMWARE), APP_NAME " " APP_VERSION);
  rows::info(list, ICON_HISTORY, core::tr(core::Str::ABOUT_BUILD), __DATE__);
  rows::info(list, ICON_CHIP, core::tr(core::Str::ABOUT_BOARD), info.board_name);
  rows::info(list, ICON_DASHBOARD, core::tr(core::Str::ABOUT_DISPLAY), info.panel_name);
  snprintf(buf, sizeof(buf), "%s · %lu MHz", info.chip_model, static_cast<unsigned long>(info.cpu_mhz));
  rows::info(list, ICON_CHIP, core::tr(core::Str::ABOUT_CHIP), buf);
  snprintf(buf, sizeof(buf), "%lu MB", static_cast<unsigned long>(info.flash_bytes / (1024 * 1024)));
  rows::info(list, ICON_DATABASE, core::tr(core::Str::ABOUT_FLASH), buf);
  memory_label_ = rows::info(list, ICON_MEMORY, core::tr(core::Str::ABOUT_MEMORY), "");
  uptime_label_ = rows::info(list, ICON_TIMER, core::tr(core::Str::ABOUT_UPTIME), "");
  snprintf(buf, sizeof(buf), "LVGL %d.%d.%d", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);
  rows::info(list, ICON_PALETTE, core::tr(core::Str::ABOUT_GRAPHICS), buf);
}

}  // namespace ui
