#include "touch.h"

#include "../core/log.h"
#include "../core/persist.h"
#include "../core/util.h"
#include "display.h"
#include "lcd.h"

namespace hal::touch {
namespace {

// A press must be seen on two consecutive reads before it is reported, which
// suppresses the spikes typical for resistive panels.
constexpr uint8_t kPressDebounce = 2;

/** Persisted calibration, tied to the display/touch orientation it was made with. */
struct StoredCalibration {
  core::TouchCalibration cal;
  uint32_t orientation;  ///< lcd_orientation_id() at calibration time
};

core::Storage* g_storage = nullptr;
lv_indev_t* g_indev = nullptr;
core::TouchCalibration g_cal = core::TouchCalibration::identity();  // landscape base orientation
bool g_calibrated = false;
bool g_enabled = true;
uint8_t g_press_count = 0;
lv_point_t g_last{0, 0};

/** Correction for the current orientation. */
core::TouchCalibration active_calibration() {
  if (!display::flipped()) return g_cal;
  return g_cal.rotated_180(lcd().width(), lcd().height());
}

void read_cb(lv_indev_t*, lv_indev_data_t* data) {
  int32_t x = 0;
  int32_t y = 0;
  const bool pressed = g_enabled && lcd().getTouch(&x, &y) > 0;

  if (!pressed) {
    g_press_count = 0;
    data->state = LV_INDEV_STATE_RELEASED;
    data->point = g_last;
    return;
  }
  if (g_press_count < kPressDebounce) g_press_count++;
  if (g_press_count < kPressDebounce) {
    data->state = LV_INDEV_STATE_RELEASED;
    data->point = g_last;
    return;
  }

  const core::TouchPoint p = active_calibration().map(x, y);
  g_last.x = core::clamp<int32_t>(p.x, 0, lcd().width() - 1);
  g_last.y = core::clamp<int32_t>(p.y, 0, lcd().height() - 1);
  data->point = g_last;
  data->state = LV_INDEV_STATE_PRESSED;
}

}  // namespace

void init(core::Storage& storage) {
  g_storage = &storage;

  StoredCalibration stored{core::TouchCalibration::identity(), 0};
  const auto result = core::persist::load(storage, core::storage_key::kTouchCal, &stored, sizeof(stored));
  if (result == core::persist::LoadResult::Missing) {
    LOG_I("TOUCH", "no calibration stored, using nominal mapping");
  } else if (result != core::persist::LoadResult::Ok) {
    LOG_W("TOUCH", "stored calibration outdated or invalid, recalibration required");
  } else if (stored.orientation != lcd_orientation_id()) {
    // Made for another panel type / orientation (board_config.h changed): recalibrate.
    LOG_W("TOUCH", "calibration belongs to another display orientation, recalibration required");
  } else if (!stored.cal.plausible()) {
    LOG_W("TOUCH", "stored calibration implausible, recalibration required");
  } else {
    g_cal = stored.cal;
    g_calibrated = true;
    LOG_I("TOUCH", "calibration loaded");
  }

  g_indev = lv_indev_create();
  lv_indev_set_type(g_indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(g_indev, read_cb);
}

bool calibrated() { return g_calibrated; }

bool read_raw(platform::TouchSample& out) {
  int32_t x = 0;
  int32_t y = 0;
  out.pressed = lcd().getTouch(&x, &y) > 0;
  out.raw_x = static_cast<int16_t>(x);
  out.raw_y = static_cast<int16_t>(y);
  out.pressure = out.pressed ? 1 : 0;
  return true;
}

void apply_calibration(const core::TouchCalibration& cal) {
  // Store in the base orientation so the correction survives a rotation change.
  const core::TouchCalibration base = display::flipped() ? cal.rotated_180(lcd().width(), lcd().height()) : cal;
  if (!base.plausible()) {
    LOG_W("TOUCH", "rejected implausible calibration");
    return;
  }
  g_cal = base;
  g_calibrated = true;
  if (g_storage != nullptr) {
    const StoredCalibration stored{g_cal, lcd_orientation_id()};
    core::persist::save(*g_storage, core::storage_key::kTouchCal, core::kTouchCalibrationVersion, &stored,
                        sizeof(stored));
  }
  LOG_I("TOUCH", "calibration saved: a=%.3f b=%.3f c=%.1f d=%.3f e=%.3f f=%.1f", g_cal.a, g_cal.b, g_cal.c, g_cal.d,
        g_cal.e, g_cal.f);
}

void set_enabled(bool enabled) {
  g_enabled = enabled;
  g_press_count = 0;
  if (g_indev != nullptr) lv_indev_reset(g_indev, nullptr);
}

}  // namespace hal::touch
