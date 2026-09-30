#include "sim_link.h"

#include <math.h>

#include "util.h"

namespace core {
namespace {

constexpr uint32_t kStepMs = 50;         // 20 Hz model / publish rate
constexpr uint32_t kConnectDelayMs = 1200;
constexpr uint32_t kReadDurationMs = 1700;
constexpr uint32_t kClearDurationMs = 1400;

// Engine speed per km/h for gears 1..6 (index 0 = neutral).
constexpr float kRpmPerKmh[] = {0.0f, 118.0f, 69.0f, 48.0f, 37.0f, 30.0f, 25.0f};
constexpr int kTopGear = 6;
constexpr float kIdleRpmWarm = 790.0f;
constexpr float kIdleRpmCold = 1080.0f;
constexpr float kTankLitres = 50.0f;
constexpr uint16_t kClockStartMinutes = 8 * 60 + 30;

}  // namespace

float SimLink::random01() {
  // xorshift32
  rng_ ^= rng_ << 13;
  rng_ ^= rng_ >> 17;
  rng_ ^= rng_ << 5;
  return static_cast<float>(rng_ & 0xFFFFFF) / static_cast<float>(0xFFFFFF);
}

void SimLink::begin(VehicleState& state, uint32_t now_ms) {
  start_ms_ = now_ms;
  last_step_ms_ = now_ms;
  rng_ ^= now_ms * 2654435761u;

  speed_ = target_speed_ = rpm_ = throttle_ = throttle_target_ = load_ = 0.0f;
  coolant_ = oil_ = ambient_ + 2.0f;
  intake_ = ambient_ + 4.0f;
  battery_ = 12.5f;
  fuel_used_l_ = 0.0f;
  gear_ = 0;
  enter(Phase::Off, now_ms);

  // Sample fault codes (typical for a used petrol car).
  codes_[0] = {0x0301, DtcStatus::Stored};
  codes_[1] = {0x0420, DtcStatus::Stored};
  codes_[2] = {0x0171, DtcStatus::Pending};
  code_count_ = 3;
  mil_ = true;
  pending_ = DtcOperation::Idle;

  state.simulated = true;
  state.link_state = LinkState::Connecting;
  state.clock_valid = false;
}

void SimLink::enter(Phase phase, uint32_t now_ms) {
  phase_ = phase;
  switch (phase) {
    case Phase::Off:
      phase_until_ms_ = now_ms + kConnectDelayMs + 400;
      break;
    case Phase::Cranking:
      phase_until_ms_ = now_ms + 900;
      break;
    case Phase::Idle:
      target_speed_ = 0.0f;
      phase_until_ms_ = now_ms + static_cast<uint32_t>(random(3500.0f, 7000.0f));
      break;
    case Phase::Accelerate:
      // Mostly relaxed driving, with an occasional sporty run up to the shift point.
      aggressiveness_ = random01() < 0.3f ? random(0.92f, 1.0f) : random(0.05f, 0.75f);
      if (speed_ < 5.0f || target_speed_ <= speed_) target_speed_ = random(55.0f, 135.0f);
      phase_until_ms_ = now_ms + 60000;  // safety limit
      break;
    case Phase::Cruise:
      phase_until_ms_ = now_ms + static_cast<uint32_t>(random(6000.0f, 14000.0f));
      break;
    case Phase::Decelerate:
      target_speed_ = (random01() < 0.55f || speed_ < 45.0f) ? 0.0f : random(35.0f, speed_ - 15.0f);
      phase_until_ms_ = now_ms + 60000;
      break;
  }
}

void SimLink::update_gear() {
  if (speed_ < 2.5f) {
    gear_ = 0;
    return;
  }
  if (gear_ == 0) gear_ = 1;

  const float upshift_rpm = 2300.0f + aggressiveness_ * 4200.0f;
  switch (phase_) {
    case Phase::Accelerate:
      if (gear_ < kTopGear && speed_ * kRpmPerKmh[gear_] > upshift_rpm) gear_++;
      break;
    case Phase::Cruise:
      while (gear_ < kTopGear && speed_ * kRpmPerKmh[gear_ + 1] > 1550.0f) gear_++;
      break;
    default:
      break;
  }
  // Never lug the engine.
  while (gear_ > 1 && speed_ * kRpmPerKmh[gear_] < 1150.0f) gear_--;
}

void SimLink::simulate(float dt, uint32_t now_ms) {
  const bool running = phase_ != Phase::Off && phase_ != Phase::Cranking;

  // ---- Driver -----------------------------------------------------------------
  switch (phase_) {
    case Phase::Off:
      throttle_target_ = 0.0f;
      if (now_ms >= phase_until_ms_) enter(Phase::Cranking, now_ms);
      break;
    case Phase::Cranking:
      if (now_ms >= phase_until_ms_) {
        engine_start_ms_ = now_ms;
        enter(Phase::Idle, now_ms);
      }
      break;
    case Phase::Idle:
      throttle_target_ = 0.0f;
      if (now_ms >= phase_until_ms_) enter(Phase::Accelerate, now_ms);
      break;
    case Phase::Accelerate:
      throttle_target_ = 32.0f + aggressiveness_ * 60.0f;
      if (speed_ >= target_speed_ || now_ms >= phase_until_ms_) enter(Phase::Cruise, now_ms);
      break;
    case Phase::Cruise:
      throttle_target_ = 9.0f + speed_ * 0.13f + 3.0f * sinf(now_ms / 1300.0f);
      if (now_ms >= phase_until_ms_) {
        if (random01() < 0.3f && target_speed_ < 120.0f) {
          target_speed_ = random(target_speed_ + 15.0f, 150.0f);
          enter(Phase::Accelerate, now_ms);
        } else {
          enter(Phase::Decelerate, now_ms);
        }
      }
      break;
    case Phase::Decelerate:
      throttle_target_ = 0.0f;
      if (speed_ <= target_speed_ + 0.5f) {
        if (target_speed_ <= 0.0f) {
          speed_ = 0.0f;
          enter(Phase::Idle, now_ms);
        } else {
          enter(Phase::Cruise, now_ms);
        }
      }
      break;
  }
  throttle_ = ema(throttle_, throttle_target_, clamp(dt * 6.0f, 0.0f, 1.0f));

  // ---- Longitudinal dynamics (km/h per second) --------------------------------
  const float drag = 0.00011f * speed_ * speed_ + 0.12f;
  float accel = 0.0f;
  if (phase_ == Phase::Accelerate) {
    const float gear_factor = gear_ > 0 ? powf(static_cast<float>(gear_), 0.55f) : 1.0f;
    accel = throttle_ / 100.0f * 11.0f / gear_factor - drag;
  } else if (phase_ == Phase::Cruise) {
    accel = clamp((target_speed_ - speed_) * 0.4f, -1.5f, 1.5f);
  } else if (phase_ == Phase::Decelerate) {
    accel = -(2.2f + speed_ * 0.025f);
  } else {
    accel = -4.0f;
  }
  speed_ = clamp(speed_ + accel * dt, 0.0f, 230.0f);
  update_gear();

  // ---- Engine ------------------------------------------------------------------
  float rpm_target = 0.0f;
  if (phase_ == Phase::Cranking) {
    rpm_target = random(210.0f, 280.0f);
  } else if (running) {
    const float idle = coolant_ < 45.0f ? kIdleRpmCold : kIdleRpmWarm;
    rpm_target = gear_ == 0 ? idle + throttle_ * 25.0f : fmaxf(idle, speed_ * kRpmPerKmh[gear_]);
    rpm_target += random(-12.0f, 12.0f);
  }
  rpm_ = ema(rpm_, rpm_target, clamp(dt * 9.0f, 0.0f, 1.0f));

  const bool fuel_cut = running && throttle_ < 1.0f && rpm_ > 1150.0f && speed_ > 8.0f;
  if (!running) {
    load_ = 0.0f;
  } else if (fuel_cut) {
    load_ = ema(load_, random(4.0f, 9.0f), 0.3f);
  } else {
    const float target_load = 17.0f + throttle_ * 0.78f + rpm_ / 8000.0f * 6.0f;
    load_ = ema(load_, clamp(target_load + random(-1.5f, 1.5f), 0.0f, 100.0f), clamp(dt * 5.0f, 0.0f, 1.0f));
  }

  // ---- Thermal model (accelerated warm-up for the demo) ------------------------
  if (running) {
    const float coolant_target = 89.0f + load_ * 0.05f + (speed_ < 5.0f ? 2.5f : 0.0f);
    coolant_ = ema(coolant_, coolant_target, dt / 55.0f);
    oil_ = ema(oil_, coolant_ + 5.0f + load_ * 0.08f, dt / 80.0f);
  } else {
    coolant_ = ema(coolant_, ambient_, dt / 600.0f);
    oil_ = ema(oil_, ambient_, dt / 900.0f);
  }
  const float intake_target = ambient_ + 7.0f + (speed_ < 10.0f ? 11.0f : 0.0f) - speed_ * 0.02f;
  intake_ = ema(intake_, intake_target, dt / 20.0f);

  // ---- Electrical ----------------------------------------------------------------
  if (phase_ == Phase::Cranking) {
    battery_ = random(10.6f, 10.9f);
  } else if (running) {
    battery_ = ema(battery_, 14.15f - (load_ > 80.0f ? 0.1f : 0.0f) + random(-0.03f, 0.03f), 0.2f);
  } else {
    battery_ = ema(battery_, 12.45f, 0.05f);
  }

  // ---- Fuel ------------------------------------------------------------------------
  stft_ = ema(stft_, random(-3.5f, 3.5f), 0.1f);
  ltft_ = ema(ltft_, 2.3f + random(-0.4f, 0.4f), 0.01f);
}

void SimLink::publish(VehicleState& s, uint32_t now) {
  const bool running = phase_ != Phase::Off && phase_ != Phase::Cranking;
  const bool fuel_cut = running && throttle_ < 1.0f && rpm_ > 1150.0f && speed_ > 8.0f;

  const float map_kpa = running ? (fuel_cut ? 24.0f : 27.0f + load_ * 1.22f) : 101.0f;
  const float maf = running ? rpm_ / 120.0f * 1.6f * 1.2f * 0.85f * (map_kpa / 101.0f) : 0.0f;
  const float fuel_rate = (running && !fuel_cut) ? maf / 14.7f * 3600.0f / 745.0f : 0.0f;
  const float timing = running ? clamp(9.0f + rpm_ / 8000.0f * 24.0f - load_ * 0.11f + random(-0.6f, 0.6f), -5.0f, 45.0f) : 0.0f;

  fuel_used_l_ += fuel_rate * (kStepMs / 3600000.0f);

  s.set(SignalId::Speed, speed_, now);
  s.set(SignalId::Rpm, rpm_, now);
  s.set(SignalId::CoolantTemp, coolant_, now);
  s.set(SignalId::OilTemp, oil_, now);
  s.set(SignalId::IntakeTemp, intake_, now);
  s.set(SignalId::AmbientTemp, ambient_, now);
  s.set(SignalId::BatteryVoltage, battery_, now);
  s.set(SignalId::FuelLevel, clamp(fuel_start_ - fuel_used_l_ / kTankLitres * 100.0f, 0.0f, 100.0f), now);
  s.set(SignalId::EngineLoad, load_, now);
  s.set(SignalId::ThrottlePos, throttle_, now);
  s.set(SignalId::IntakePressure, map_kpa, now);
  s.set(SignalId::Maf, maf, now);
  s.set(SignalId::TimingAdvance, timing, now);
  s.set(SignalId::FuelRate, fuel_rate, now);
  s.set(SignalId::ShortFuelTrim, running ? stft_ : 0.0f, now);
  s.set(SignalId::LongFuelTrim, ltft_, now);
  s.set(SignalId::RunTime, running ? elapsed(now, engine_start_ms_) / 1000.0f : 0.0f, now);
  s.set(SignalId::Gear, static_cast<float>(gear_), now);

  s.clock_valid = true;
  s.clock_minutes = static_cast<uint16_t>((kClockStartMinutes + elapsed(now, start_ms_) / 60000) % (24 * 60));

  if (!s.dtc.mil_known || s.dtc.mil_on != mil_) {
    s.dtc.mil_known = true;
    s.dtc.mil_on = mil_;
    s.dtc.revision++;
  }
}

void SimLink::poll(VehicleState& state, uint32_t now_ms) {
  if (state.link_state == LinkState::Connecting && elapsed(now_ms, start_ms_) >= kConnectDelayMs) {
    state.link_state = LinkState::Connected;
  }

  if (pending_ != DtcOperation::Idle && static_cast<int32_t>(now_ms - pending_done_ms_) >= 0) {
    complete_dtc_operation(state, now_ms);
  }

  // Fixed-step integration keeps the model independent of the loop rate.
  uint8_t guard = 0;
  while (elapsed(now_ms, last_step_ms_) >= kStepMs && guard++ < 10) {
    last_step_ms_ += kStepMs;
    simulate(kStepMs / 1000.0f, last_step_ms_);
    if (state.link_state == LinkState::Connected) publish(state, last_step_ms_);
  }
  if (guard >= 10) last_step_ms_ = now_ms;  // we were stalled, do not try to catch up
}

bool SimLink::request_dtc_read(VehicleState& state, uint32_t now_ms) {
  if (state.link_state != LinkState::Connected) {
    state.dtc.last_result = DtcResult::NotConnected;
    state.dtc.revision++;
    return false;
  }
  if (pending_ != DtcOperation::Idle) return false;
  pending_ = DtcOperation::Reading;
  pending_done_ms_ = now_ms + kReadDurationMs;
  state.dtc.operation = DtcOperation::Reading;
  state.dtc.revision++;
  return true;
}

bool SimLink::request_dtc_clear(VehicleState& state, uint32_t now_ms) {
  if (state.link_state != LinkState::Connected) {
    state.dtc.last_result = DtcResult::NotConnected;
    state.dtc.revision++;
    return false;
  }
  if (pending_ != DtcOperation::Idle) return false;
  pending_ = DtcOperation::Clearing;
  pending_done_ms_ = now_ms + kClearDurationMs;
  state.dtc.operation = DtcOperation::Clearing;
  state.dtc.revision++;
  return true;
}

void SimLink::complete_dtc_operation(VehicleState& state, uint32_t now_ms) {
  DtcState& dtc = state.dtc;
  if (pending_ == DtcOperation::Clearing) {
    code_count_ = 0;
    mil_ = false;
    dtc.last_result = DtcResult::ClearOk;
  } else {
    dtc.last_result = DtcResult::ReadOk;
  }
  dtc.count = code_count_;
  for (uint8_t i = 0; i < code_count_; ++i) dtc.codes[i] = codes_[i];
  dtc.mil_on = mil_;
  dtc.mil_known = true;
  dtc.scanned = true;
  dtc.last_scan_ms = now_ms;
  dtc.operation = DtcOperation::Idle;
  dtc.revision++;
  pending_ = DtcOperation::Idle;
}

}  // namespace core
