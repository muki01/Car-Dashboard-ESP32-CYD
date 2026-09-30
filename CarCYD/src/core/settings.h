/**
 * @file settings.h
 * User settings model with change notification and deferred persistence.
 *
 * Usage:
 *   auto& s = core::settings::edit();
 *   s.brightness = 70;
 *   core::settings::commit(core::settings::kChangeDisplay);
 *
 * commit() notifies listeners immediately (live preview) and schedules a save
 * shortly after the last change, which keeps flash wear low while a slider is
 * being dragged.
 */
#pragma once

#include <stdint.h>

#include "storage.h"
#include "types.h"

namespace core {

constexpr uint8_t kCornerSlots = 4;
constexpr uint8_t kGaugeSlots = 6;

/**
 * Persisted settings. Trivially copyable; NEVER reorder or remove fields -
 * append new ones at the end and bump kSettingsVersion (see persist.h).
 */
struct Settings {
  // Display
  uint8_t brightness;        ///< % (manual level, day level when auto)
  uint8_t night_brightness;  ///< % used in the dark when auto brightness is on
  bool auto_brightness;
  bool flip_screen;
  Accent accent;
  Language language;
  // Units
  SpeedUnit speed_unit;
  TempUnit temp_unit;
  PressureUnit pressure_unit;
  // Gauges
  uint16_t rpm_max;      ///< tachometer full scale
  uint16_t redline_rpm;  ///< start of the red zone
  bool shift_light;
  uint16_t shift_rpm;
  // Warnings
  uint8_t coolant_warn_c;   ///< degC
  uint8_t low_voltage_dv;   ///< decivolts (118 = 11.8 V)
  uint8_t fuel_warn_pct;    ///< %, 0 = off
  uint16_t overspeed_kmh;   ///< km/h, 0 = off
  // Sound & light
  bool touch_sound;
  bool alert_sound;
  uint8_t volume;  ///< %
  bool status_led;
  // Data source
  bool demo_mode;
  // Dashboard layout
  uint8_t dash_view;
  SignalId corner_slots[kCornerSlots];
  SignalId gauge_slots[kGaugeSlots];
};

constexpr uint16_t kSettingsVersion = 1;

namespace settings {

/** Bit mask describing what changed in a commit. */
enum Change : uint32_t {
  kChangeDisplay = 1u << 0,      ///< brightness related
  kChangeOrientation = 1u << 1,  ///< screen rotation
  kChangeTheme = 1u << 2,        ///< accent colour (UI rebuild)
  kChangeLanguage = 1u << 3,     ///< language (UI rebuild)
  kChangeUnits = 1u << 4,
  kChangeGauges = 1u << 5,
  kChangeWarnings = 1u << 6,
  kChangeSound = 1u << 7,
  kChangeLed = 1u << 8,
  kChangeDataSource = 1u << 9,
  kChangeLayout = 1u << 10,  ///< dashboard view / slots
  kChangeAll = 0xFFFFFFFFu,
};

using Listener = void (*)(uint32_t changes, void* user);

const Settings& get();
Settings& edit();

/** Validates the edited values, notifies listeners and schedules a save. */
void commit(uint32_t changes);

bool add_listener(Listener listener, void* user);

Settings defaults();

/** Binds the storage backend and loads the settings. Returns false if defaults are used. */
bool load(Storage& storage);

/** Writes pending changes immediately (e.g. before a restart). */
void flush();

/** Call periodically; performs the deferred save. */
void service(uint32_t now_ms);

/** Restores factory defaults and notifies all listeners. */
void restore_defaults();

}  // namespace settings
}  // namespace core
