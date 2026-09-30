/**
 * @file touch.h
 * LVGL pointer input for the XPT2046 resistive touch panel, with a per-unit
 * affine correction (user calibration) and press debouncing.
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

#include "../core/platform.h"
#include "../core/storage.h"
#include "../core/touch_calibration.h"

namespace hal::touch {

/** Creates the LVGL input device and loads the stored calibration. */
void init(core::Storage& storage);

/** True if a user calibration is stored (false on first boot). */
bool calibrated();

/** Sample in the current screen orientation, before the user correction. */
bool read_raw(platform::TouchSample& out);

/** Applies a correction computed in the current orientation and stores it. */
void apply_calibration(const core::TouchCalibration& cal);

void set_enabled(bool enabled);

}  // namespace hal::touch
