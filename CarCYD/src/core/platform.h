/**
 * @file platform.h
 * Port interface between the portable code (core + ui) and the hardware.
 *
 * Implemented by src/hal/platform_esp32.cpp (hardware) and src/app/app.cpp
 * (restart / factory reset) on the device. Keeping this surface small is what
 * allows the complete UI to run in a PC simulator.
 */
#pragma once

#include <stdint.h>

#include "touch_calibration.h"

namespace platform {

uint32_t millis();

// ---- System information ---------------------------------------------------------

struct SystemInfo {
  const char* chip_model;
  uint8_t chip_revision;
  uint8_t cores;
  uint32_t cpu_mhz;
  uint32_t flash_bytes;
  uint32_t heap_free;
  uint32_t heap_min_free;
  uint32_t heap_total;
  const char* sdk_version;
  const char* board_name;
  const char* panel_name;
};

void system_info(SystemInfo& out);

/** Persists pending data and restarts the device. */
[[noreturn]] void restart();

/** Erases all persisted data (settings, calibration, trip) and restarts. */
[[noreturn]] void factory_reset();

// ---- Ambient light / backlight -------------------------------------------------

/** Filtered raw reading of the light sensor (ADC counts). */
uint16_t light_sensor_raw();

/** Ambient light level 0 (dark) .. 100 (bright). */
uint8_t ambient_level();

/** Backlight level currently applied, 0..100 %. */
uint8_t backlight_level();

// ---- Audio feedback --------------------------------------------------------------

enum class Sound : uint8_t { Click, Confirm, Error, Warning, Critical, Startup };

/** Plays a UI sound, honouring the user's sound settings. Non-blocking. */
void play(Sound sound);

// ---- Touch calibration support ------------------------------------------------------

struct TouchSample {
  int16_t raw_x;
  int16_t raw_y;
  uint16_t pressure;
  bool pressed;
};

/** Reads an unfiltered-by-calibration sample from the touch controller. */
bool touch_read_raw(TouchSample& out);

/** Applies (and persists) a new calibration. */
void touch_apply_calibration(const core::TouchCalibration& cal);

/** Enables/disables normal pointer input (disabled while calibrating). */
void touch_input_enable(bool enable);

}  // namespace platform
