/**
 * @file trip.h
 * Trip computer: distance, drive time, speeds, fuel and 0-100 timer.
 *
 * Fuel is integrated from the reported fuel rate; if the vehicle does not
 * provide it, it is estimated from the mass air flow (stoichiometric petrol).
 */
#pragma once

#include <stdint.h>

#include "settings.h"
#include "vehicle_state.h"

namespace core {

/** Persisted trip data. Trivially copyable, append-only (see persist.h). */
struct TripData {
  float distance_km;
  float fuel_litres;
  float max_speed_kmh;
  uint32_t drive_time_s;   ///< engine running
  uint32_t moving_time_s;  ///< vehicle moving
  float accel_last_s;      ///< 0 = none
  float accel_best_s;      ///< 0 = none
};

constexpr uint16_t kTripVersion = 1;

enum class AccelState : uint8_t { Idle, Armed, Running };

class TripComputer {
 public:
  void update(const VehicleState& state, const Settings& settings, uint32_t now_ms);
  void reset();

  const TripData& data() const { return data_; }
  void restore(const TripData& data);

  float average_speed_kmh() const;

  AccelState accel_state() const { return accel_state_; }
  /** Running time of the current acceleration run in seconds. */
  float accel_elapsed_s(uint32_t now_ms) const;

  /** True if data changed since the last call (used for periodic persistence). */
  bool take_dirty();

 private:
  TripData data_{};
  uint32_t last_ms_ = 0;
  bool has_last_ = false;
  uint32_t drive_ms_acc_ = 0;
  uint32_t moving_ms_acc_ = 0;
  AccelState accel_state_ = AccelState::Idle;
  uint32_t accel_start_ms_ = 0;
  uint32_t stopped_since_ms_ = 0;
  bool dirty_ = false;
};

TripComputer& trip();

}  // namespace core
