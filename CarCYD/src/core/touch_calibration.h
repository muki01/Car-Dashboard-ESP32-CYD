/**
 * @file touch_calibration.h
 * Affine touch correction for resistive panels.
 *
 *   x' = a * x + b * y + c
 *   y' = d * x + e * y + f
 *
 * The display driver already maps the XPT2046 readings with the board's
 * nominal calibration; this matrix corrects the deviations of the individual
 * unit (offset, scale, skew, rotation). It is solved from three reference
 * points (C. Vidales, "How to calibrate touch screens", Embedded Systems
 * Programming, 2002) and stored for the landscape base orientation.
 */
#pragma once

#include <stdint.h>

namespace core {

struct TouchPoint {
  int32_t x;
  int32_t y;
};

struct TouchCalibration {
  float a, b, c, d, e, f;

  /** No correction. */
  static TouchCalibration identity();

  /** Computes the matrix. Returns false if the points are (nearly) collinear. */
  static bool solve(const TouchPoint raw[3], const TouchPoint screen[3], TouchCalibration& out);

  /** Same correction expressed for a display rotated by 180 degrees (w x h pixels). */
  TouchCalibration rotated_180(int32_t width, int32_t height) const;

  /** Plausibility check (rejects corrupt or absurd matrices). */
  bool plausible() const;

  TouchPoint map(int32_t x, int32_t y) const;
};

/** 2: stored together with the display orientation id (hal/touch.cpp). */
constexpr uint16_t kTouchCalibrationVersion = 2;

}  // namespace core
