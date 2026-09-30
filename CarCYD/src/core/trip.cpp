#include "trip.h"

#include <type_traits>

#include "units.h"
#include "util.h"

namespace core {
namespace {

static_assert(std::is_trivially_copyable<TripData>::value, "TripData must stay trivially copyable");

constexpr float kStoichiometricAfr = 14.7f;
constexpr float kPetrolDensityGPerL = 745.0f;
constexpr float kMovingKmh = 2.0f;
constexpr uint32_t kArmDelayMs = 500;
constexpr uint32_t kAccelTimeoutMs = 30000;
constexpr uint32_t kMaxStepMs = 1000;  // ignore gaps (e.g. link loss)

}  // namespace

void TripComputer::reset() {
  data_ = TripData{};
  drive_ms_acc_ = moving_ms_acc_ = 0;
  accel_state_ = AccelState::Idle;
  dirty_ = true;
}

void TripComputer::restore(const TripData& data) {
  data_ = data;
  dirty_ = false;
}

float TripComputer::average_speed_kmh() const {
  return data_.moving_time_s > 0 ? data_.distance_km / (data_.moving_time_s / 3600.0f) : 0.0f;
}

float TripComputer::accel_elapsed_s(uint32_t now_ms) const {
  return accel_state_ == AccelState::Running ? elapsed(now_ms, accel_start_ms_) / 1000.0f : 0.0f;
}

bool TripComputer::take_dirty() {
  const bool was = dirty_;
  dirty_ = false;
  return was;
}

void TripComputer::update(const VehicleState& state, const Settings& settings, uint32_t now) {
  const uint32_t dt_ms = has_last_ ? elapsed(now, last_ms_) : 0;
  last_ms_ = now;
  has_last_ = true;
  if (dt_ms == 0 || dt_ms > kMaxStepMs || state.link_state != LinkState::Connected) return;

  const bool speed_ok = state.fresh(SignalId::Speed, now);
  const float speed = speed_ok ? state.get(SignalId::Speed).value : 0.0f;
  const float dt_h = dt_ms / 3600000.0f;

  // ---- Distance, time and speeds ---------------------------------------------
  if (speed_ok && speed > 0.0f) {
    data_.distance_km += speed * dt_h;
    if (speed > data_.max_speed_kmh) data_.max_speed_kmh = speed;
    dirty_ = true;
  }
  if (state.engine_running(now)) {
    drive_ms_acc_ += dt_ms;
    data_.drive_time_s += drive_ms_acc_ / 1000;
    drive_ms_acc_ %= 1000;
  }
  if (speed > kMovingKmh) {
    moving_ms_acc_ += dt_ms;
    data_.moving_time_s += moving_ms_acc_ / 1000;
    moving_ms_acc_ %= 1000;
  }

  // ---- Fuel ------------------------------------------------------------------------
  if (state.fresh(SignalId::FuelRate, now)) {
    data_.fuel_litres += state.get(SignalId::FuelRate).value * dt_h;
  } else if (state.fresh(SignalId::Maf, now)) {
    const float grams_per_s = state.get(SignalId::Maf).value / kStoichiometricAfr;
    data_.fuel_litres += grams_per_s / kPetrolDensityGPerL * (dt_ms / 1000.0f);
  }

  // ---- Acceleration timer (0-100 km/h or 0-60 mph) -------------------------------
  const float target = units::accel_target_kmh(settings);
  switch (accel_state_) {
    case AccelState::Idle:
      if (speed_ok && speed < 0.5f) {
        if (stopped_since_ms_ == 0) stopped_since_ms_ = now;
        if (elapsed(now, stopped_since_ms_) >= kArmDelayMs) accel_state_ = AccelState::Armed;
      } else {
        stopped_since_ms_ = 0;
      }
      break;
    case AccelState::Armed:
      if (speed_ok && speed >= 0.5f) {
        accel_state_ = AccelState::Running;
        accel_start_ms_ = now;
      }
      break;
    case AccelState::Running: {
      const uint32_t run_ms = elapsed(now, accel_start_ms_);
      if (speed_ok && speed >= target) {
        data_.accel_last_s = run_ms / 1000.0f;
        if (data_.accel_best_s <= 0.0f || data_.accel_last_s < data_.accel_best_s) {
          data_.accel_best_s = data_.accel_last_s;
        }
        dirty_ = true;
        accel_state_ = AccelState::Idle;
        stopped_since_ms_ = 0;
      } else if (!speed_ok || speed < 0.5f || run_ms > kAccelTimeoutMs) {
        accel_state_ = AccelState::Idle;  // aborted
        stopped_since_ms_ = 0;
      }
      break;
    }
  }
}

TripComputer& trip() {
  static TripComputer computer;
  return computer;
}

}  // namespace core
