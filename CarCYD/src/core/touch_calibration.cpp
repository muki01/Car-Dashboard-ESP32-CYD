#include "touch_calibration.h"

#include <math.h>

namespace core {

TouchCalibration TouchCalibration::identity() { return TouchCalibration{1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f}; }

bool TouchCalibration::solve(const TouchPoint raw[3], const TouchPoint screen[3], TouchCalibration& out) {
  const double xs0 = raw[0].x, ys0 = raw[0].y;
  const double xs1 = raw[1].x, ys1 = raw[1].y;
  const double xs2 = raw[2].x, ys2 = raw[2].y;
  const double xd0 = screen[0].x, yd0 = screen[0].y;
  const double xd1 = screen[1].x, yd1 = screen[1].y;
  const double xd2 = screen[2].x, yd2 = screen[2].y;

  const double k = (xs0 - xs2) * (ys1 - ys2) - (xs1 - xs2) * (ys0 - ys2);
  if (fabs(k) < 1.0) return false;

  out.a = static_cast<float>(((xd0 - xd2) * (ys1 - ys2) - (xd1 - xd2) * (ys0 - ys2)) / k);
  out.b = static_cast<float>(((xs0 - xs2) * (xd1 - xd2) - (xd0 - xd2) * (xs1 - xs2)) / k);
  out.c = static_cast<float>((ys0 * (xs2 * xd1 - xs1 * xd2) + ys1 * (xs0 * xd2 - xs2 * xd0) +
                              ys2 * (xs1 * xd0 - xs0 * xd1)) / k);
  out.d = static_cast<float>(((yd0 - yd2) * (ys1 - ys2) - (yd1 - yd2) * (ys0 - ys2)) / k);
  out.e = static_cast<float>(((xs0 - xs2) * (yd1 - yd2) - (yd0 - yd2) * (xs1 - xs2)) / k);
  out.f = static_cast<float>((ys0 * (xs2 * yd1 - xs1 * yd2) + ys1 * (xs0 * yd2 - xs2 * yd0) +
                              ys2 * (xs1 * yd0 - xs0 * yd1)) / k);
  return true;
}

TouchCalibration TouchCalibration::rotated_180(int32_t width, int32_t height) const {
  // R(p) = (W - x, H - y) with W = width - 1, H = height - 1.  R o M o R keeps the
  // linear part and only moves the offsets.
  const float w = static_cast<float>(width - 1);
  const float h = static_cast<float>(height - 1);
  return TouchCalibration{a, b, w - a * w - b * h - c, d, e, h - d * w - e * h - f};
}

bool TouchCalibration::plausible() const {
  const float values[] = {a, b, c, d, e, f};
  for (float v : values) {
    if (!isfinite(v)) return false;
  }
  // Any orientation is allowed (axis swap / mirror), but the scale must stay in a
  // sane range and the offsets within a few screen sizes.
  const float det = fabsf(a * e - b * d);
  return det > 0.25f && det < 4.0f && fabsf(c) < 1000.0f && fabsf(f) < 1000.0f;
}

TouchPoint TouchCalibration::map(int32_t x, int32_t y) const {
  const float mx = a * x + b * y + c;
  const float my = d * x + e * y + f;
  return TouchPoint{static_cast<int32_t>(lroundf(mx)), static_cast<int32_t>(lroundf(my))};
}

}  // namespace core
