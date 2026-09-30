/**
 * @file sim_link.h
 * Demo data source: a small vehicle model that drives through realistic
 * cycles (cold start, idle, acceleration with gear shifts, cruise, braking)
 * and answers diagnostic requests with a set of sample fault codes.
 */
#pragma once

#include "data_link.h"

namespace core {

class SimLink final : public DataLink {
 public:
  const char* name() const override { return "demo"; }
  void begin(VehicleState& state, uint32_t now_ms) override;
  void poll(VehicleState& state, uint32_t now_ms) override;
  bool request_dtc_read(VehicleState& state, uint32_t now_ms) override;
  bool request_dtc_clear(VehicleState& state, uint32_t now_ms) override;

 private:
  enum class Phase : uint8_t { Off, Cranking, Idle, Accelerate, Cruise, Decelerate };

  void simulate(float dt_s, uint32_t now_ms);
  void enter(Phase phase, uint32_t now_ms);
  void update_gear();
  void publish(VehicleState& state, uint32_t now_ms);
  void complete_dtc_operation(VehicleState& state, uint32_t now_ms);

  float random01();
  float random(float lo, float hi) { return lo + (hi - lo) * random01(); }

  uint32_t rng_ = 0x9E3779B9u;
  uint32_t start_ms_ = 0;
  uint32_t last_step_ms_ = 0;
  uint32_t phase_until_ms_ = 0;
  uint32_t engine_start_ms_ = 0;
  Phase phase_ = Phase::Off;

  // Vehicle model (base units)
  float speed_ = 0.0f;
  float target_speed_ = 0.0f;
  float rpm_ = 0.0f;
  float throttle_ = 0.0f;
  float throttle_target_ = 0.0f;
  float aggressiveness_ = 0.5f;
  float load_ = 0.0f;
  float coolant_ = 0.0f;
  float oil_ = 0.0f;
  float intake_ = 0.0f;
  float ambient_ = 21.0f;
  float battery_ = 12.5f;
  float fuel_start_ = 62.0f;
  float fuel_used_l_ = 0.0f;
  float stft_ = 0.0f;
  float ltft_ = 2.3f;
  int gear_ = 0;

  // Diagnostics
  Dtc codes_[4] = {};
  uint8_t code_count_ = 0;
  bool mil_ = false;
  DtcOperation pending_ = DtcOperation::Idle;
  uint32_t pending_done_ms_ = 0;
};

}  // namespace core
