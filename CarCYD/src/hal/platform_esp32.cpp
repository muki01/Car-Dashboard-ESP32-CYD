/**
 * @file platform_esp32.cpp
 * Implementation of the platform port (core/platform.h) on the CYD.
 * restart() and factory_reset() live in app.cpp because they have to persist
 * application data first.
 */
#include <Arduino.h>

#include "../config/board_config.h"
#include "../core/platform.h"
#include "../core/settings.h"
#include "backlight.h"
#include "buzzer.h"
#include "lcd.h"
#include "light_sensor.h"
#include "touch.h"

namespace platform {

uint32_t millis() { return ::millis(); }

void system_info(SystemInfo& out) {
  out.chip_model = ESP.getChipModel();
  out.chip_revision = ESP.getChipRevision();
  out.cores = ESP.getChipCores();
  out.cpu_mhz = ESP.getCpuFreqMHz();
  out.flash_bytes = ESP.getFlashChipSize();
  out.heap_free = ESP.getFreeHeap();
  out.heap_min_free = ESP.getMinFreeHeap();
  out.heap_total = ESP.getHeapSize();
  out.sdk_version = ESP.getSdkVersion();
  out.board_name = CYD_BOARD_NAME;
  out.panel_name = hal::lcd_panel_name();
}

uint16_t light_sensor_raw() { return hal::light_sensor::raw(); }

uint8_t ambient_level() { return hal::light_sensor::level(); }

uint8_t backlight_level() { return hal::backlight::level(); }

void play(Sound sound) {
  const core::Settings& s = core::settings::get();
  switch (sound) {
    case Sound::Click:
      if (s.touch_sound) hal::buzzer::play(hal::buzzer::Sound::Click);
      break;
    case Sound::Confirm:
      if (s.touch_sound) hal::buzzer::play(hal::buzzer::Sound::Confirm);
      break;
    case Sound::Error:
      if (s.touch_sound) hal::buzzer::play(hal::buzzer::Sound::Error);
      break;
    case Sound::Startup:
      if (s.touch_sound) hal::buzzer::play(hal::buzzer::Sound::Startup);
      break;
    case Sound::Warning:
      if (s.alert_sound) hal::buzzer::play(hal::buzzer::Sound::Warning);
      break;
    case Sound::Critical:
      if (s.alert_sound) hal::buzzer::play(hal::buzzer::Sound::Critical);
      break;
  }
}

bool touch_read_raw(TouchSample& out) { return hal::touch::read_raw(out); }

void touch_apply_calibration(const core::TouchCalibration& cal) { hal::touch::apply_calibration(cal); }

void touch_input_enable(bool enable) { hal::touch::set_enabled(enable); }

}  // namespace platform
