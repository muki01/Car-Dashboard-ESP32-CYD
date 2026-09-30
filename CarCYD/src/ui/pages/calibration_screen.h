/**
 * @file calibration_screen.h
 * Full-screen touch calibration: three reference targets plus a verification
 * target. The result is only accepted if the verification error is small.
 */
#pragma once

#include <lvgl.h>

namespace ui::calibration {

using DoneFn = void (*)();

/** Creates and loads the calibration screen; `on_done` runs after success. */
void start(DoneFn on_done);

}  // namespace ui::calibration
