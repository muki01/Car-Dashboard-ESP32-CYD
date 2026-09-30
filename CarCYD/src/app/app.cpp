#include "app.h"

#include <Arduino.h>
#include <lvgl.h>

#include "../config/app_config.h"
#include "../config/board_config.h"
#include "../core/alerts.h"
#include "../core/data_link.h"
#include "../core/i18n.h"
#include "../core/log.h"
#include "../core/persist.h"
#include "../core/platform.h"
#include "../core/settings.h"
#include "../core/sim_link.h"
#include "../core/trip.h"
#include "../core/util.h"
#include "../core/vehicle_state.h"
#include "../hal/backlight.h"
#include "../hal/button.h"
#include "../hal/buzzer.h"
#include "../hal/display.h"
#include "../hal/light_sensor.h"
#include "../hal/nvs_storage.h"
#include "../hal/rgb_led.h"
#include "../hal/touch.h"
#include "../ui/ui.h"

#if !defined(ESP_ARDUINO_VERSION_MAJOR) || ESP_ARDUINO_VERSION_MAJOR < 3
#error "CarCYD requires the ESP32 Arduino core 3.x (Boards Manager: esp32 by Espressif Systems)"
#endif
#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR < 6
#error "CarCYD requires LVGL 9.6 or newer (9.x)"
#endif

namespace app {
namespace {

constexpr const char* kNvsNamespace = "carcyd";

hal::NvsStorage g_storage;
core::SimLink g_sim_link;
core::NullLink g_null_link;

uint32_t g_last_backlight_ms = 0;
uint32_t g_last_trip_save_ms = 0;
uint32_t g_last_critical_sound_ms = 0;
core::Severity g_sounded[core::kAlertCount] = {};

// ---- Logging -------------------------------------------------------------------------------

void serial_sink(const char* line) { Serial.println(line); }

uint32_t millis_cb() { return millis(); }

void lvgl_log_cb(lv_log_level_t, const char* text) { Serial.print(text); }

// ---- Services ------------------------------------------------------------------------------

void select_data_source(uint32_t now_ms) {
  core::DataLink* link = core::settings::get().demo_mode ? static_cast<core::DataLink*>(&g_sim_link)
                                                         : static_cast<core::DataLink*>(&g_null_link);
  core::link::set_active(link, now_ms);
  core::alerts().reset();
  for (auto& s : g_sounded) s = core::Severity::Normal;
}

uint8_t target_brightness() {
  const core::Settings& s = core::settings::get();
  if (!s.auto_brightness) return s.brightness;
  const float ambient = hal::light_sensor::level() / 100.0f;
  const float night = core::clamp<float>(s.night_brightness, 5.0f, s.brightness);
  return static_cast<uint8_t>(core::lerp(night, s.brightness, ambient) + 0.5f);
}

void update_backlight(uint32_t now_ms) {
  if (core::elapsed(now_ms, g_last_backlight_ms) < APP_BACKLIGHT_TICK_MS) return;
  g_last_backlight_ms = now_ms;
  hal::light_sensor::update(now_ms);
  hal::backlight::set_target(target_brightness());
  hal::backlight::update(now_ms);
  // The status LED follows the screen so it never dazzles at night.
  hal::rgb_led::set_intensity(core::clamp<uint8_t>(hal::backlight::level(), 10, 100));
}

void update_status_led(uint32_t now_ms) {
  using namespace hal::rgb_led;
  const core::Settings& s = core::settings::get();
  if (!s.status_led) {
    off();
    return;
  }
  const core::Severity alert = core::alerts().highest();
  const float rpm = core::vehicle().value_or(core::SignalId::Rpm, now_ms, 0.0f);

  if (alert == core::Severity::Critical) {
    set(colors::kRed, Pattern::Blink, 300);
  } else if (s.shift_light && rpm >= s.shift_rpm) {
    set(colors::kRed, Pattern::Blink, 120);
  } else if (s.shift_light && rpm >= s.shift_rpm - 600) {
    set(colors::kAmber);
  } else if (alert == core::Severity::Warning) {
    set(colors::kAmber, Pattern::Pulse, 2000);
  } else if (core::vehicle().link_state != core::LinkState::Connected) {
    set(colors::kBlue, Pattern::Pulse, 3000);
  } else {
    off();
  }
}

void update_alert_sounds(uint32_t now_ms) {
  core::AlertManager& alerts = core::alerts();
  bool critical_pending = false;
  for (uint8_t i = 0; i < core::kAlertCount; ++i) {
    const core::Alert& a = alerts.get(static_cast<core::AlertId>(i));
    if (!a.active()) {
      g_sounded[i] = core::Severity::Normal;
      continue;
    }
    if (a.acknowledged) continue;
    if (a.severity == core::Severity::Critical) critical_pending = true;
    if (a.severity > g_sounded[i]) {
      g_sounded[i] = a.severity;
      platform::play(a.severity == core::Severity::Critical ? platform::Sound::Critical : platform::Sound::Warning);
      if (a.severity == core::Severity::Critical) g_last_critical_sound_ms = now_ms;
    }
  }
  // Unacknowledged critical alerts keep reminding the driver.
  if (critical_pending && core::elapsed(now_ms, g_last_critical_sound_ms) >= APP_ALERT_REPEAT_MS) {
    g_last_critical_sound_ms = now_ms;
    platform::play(platform::Sound::Critical);
  }
}

void handle_button(uint32_t now_ms) {
  switch (hal::button::poll(now_ms)) {
    case hal::button::Event::Short:
      ui::on_hardware_button_short();
      break;
    case hal::button::Event::Long:
      ui::navigate(ui::PageId::Dashboard);
      break;
    case hal::button::Event::None:
      break;
  }
}

void save_trip(bool force) {
  if (core::trip().take_dirty() || force) {
    core::persist::save(g_storage, core::storage_key::kTrip, core::kTripVersion, &core::trip().data(),
                        sizeof(core::TripData));
  }
}

void load_trip() {
  core::TripData data{};
  const auto result = core::persist::load(g_storage, core::storage_key::kTrip, &data, sizeof(data));
  if (result == core::persist::LoadResult::Ok || result == core::persist::LoadResult::Upgraded) {
    core::trip().restore(data);
  }
}

void on_settings_changed(uint32_t changes, void*) {
  const core::Settings& s = core::settings::get();
  if (changes & core::settings::kChangeOrientation) hal::display::set_flipped(s.flip_screen);
  if (changes & core::settings::kChangeSound) hal::buzzer::set_volume(s.volume);
  if (changes & core::settings::kChangeDataSource) select_data_source(millis());
}

/** Renders the splash while the start-up continues. */
void boot_step(uint8_t percent, core::Str text) {
  ui::splash_progress(percent, core::tr(text));
  lv_timer_handler();
  lv_refr_now(nullptr);
}

void halt_with_error(const char* what) {
  LOG_E("APP", "fatal: %s", what);
  pinMode(board::kLedRed, OUTPUT);
  while (true) {
    digitalWrite(board::kLedRed, !digitalRead(board::kLedRed));
    delay(250);
  }
}

}  // namespace

void setup() {
  Serial.begin(APP_SERIAL_BAUD);
  core::log::init(serial_sink, millis_cb, static_cast<core::log::Level>(APP_LOG_LEVEL));
  LOG_I("APP", "%s %s (%s) on %s, LVGL %d.%d.%d", APP_NAME, APP_VERSION, __DATE__, CYD_BOARD_NAME,
        LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR, LVGL_VERSION_PATCH);

  hal::button::init();
  const bool calibration_requested = hal::button::held();

  // Settings first: they define orientation, language and brightness.
  g_storage.begin(kNvsNamespace);
  core::settings::load(g_storage);
  const core::Settings& s = core::settings::get();

  // ---- Graphics -----------------------------------------------------------------------
  lv_init();
  lv_tick_set_cb(millis_cb);
  lv_log_register_print_cb(lvgl_log_cb);

  hal::backlight::init();
  lv_display_t* display = hal::display::init(s.flip_screen);
  if (display == nullptr) halt_with_error("display");
  ui::init(display);
  ui::splash_show();
  lv_refr_now(nullptr);
  const uint32_t splash_start = millis();

  hal::light_sensor::init();
  hal::backlight::set_target(target_brightness());  // fades in while booting

  // ---- Hardware -------------------------------------------------------------------------
  boot_step(20, core::Str::BOOT_SETTINGS);
  hal::touch::init(g_storage);
  hal::rgb_led::init();
  hal::buzzer::init();
  hal::buzzer::set_volume(s.volume);
  boot_step(50, core::Str::BOOT_HARDWARE);

  // ---- Data -------------------------------------------------------------------------------
  load_trip();
  select_data_source(millis());
  core::settings::add_listener(on_settings_changed, nullptr);
  boot_step(80, core::Str::BOOT_VEHICLE);

  while (core::elapsed(millis(), splash_start) < APP_SPLASH_MIN_MS) {
    hal::backlight::update(millis());
    lv_timer_handler();
    delay(5);
  }
  boot_step(100, core::Str::BOOT_READY);
  platform::play(platform::Sound::Startup);

  const bool run_calibration = calibration_requested || !hal::touch::calibrated();
  if (run_calibration) LOG_I("APP", "touch calibration %s", calibration_requested ? "requested" : "required");
  ui::start(run_calibration);
  g_last_trip_save_ms = millis();
  LOG_I("APP", "ready, free heap %lu B", static_cast<unsigned long>(ESP.getFreeHeap()));
}

void loop() {
  const uint32_t now = millis();

  core::link::poll(now);
  core::alerts().update(core::vehicle(), core::settings::get(), now);
  core::trip().update(core::vehicle(), core::settings::get(), now);

  update_alert_sounds(now);
  update_status_led(now);
  update_backlight(now);
  handle_button(now);
  hal::buzzer::update(now);
  hal::rgb_led::update(now);

  core::settings::service(now);
  if (core::elapsed(now, g_last_trip_save_ms) >= APP_TRIP_SAVE_PERIOD_MS) {
    g_last_trip_save_ms = now;
    save_trip(false);
  }

  const uint32_t idle_ms = lv_timer_handler();
  delay(idle_ms > 5 ? 5 : (idle_ms == 0 ? 1 : idle_ms));
}

}  // namespace app

// ---- Platform services that need application data ------------------------------------------

namespace platform {

void restart() {
  LOG_I("APP", "restart requested");
  core::settings::flush();
  app::save_trip(true);
  delay(100);
  ESP.restart();
  while (true) {
  }
}

void factory_reset() {
  LOG_W("APP", "factory reset");
  app::g_storage.erase_all();
  delay(100);
  ESP.restart();
  while (true) {
  }
}

}  // namespace platform
