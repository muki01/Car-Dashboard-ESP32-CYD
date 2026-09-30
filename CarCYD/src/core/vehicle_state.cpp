#include "vehicle_state.h"

#include "signals.h"
#include "util.h"

namespace core {

void VehicleState::set(SignalId id, float value, uint32_t now_ms) {
  SignalValue& s = signals_[static_cast<uint8_t>(id)];
  s.value = value;
  s.updated_ms = now_ms;
  s.valid = true;
}

void VehicleState::invalidate(SignalId id) { signals_[static_cast<uint8_t>(id)].valid = false; }

void VehicleState::invalidate_all() {
  for (auto& s : signals_) s.valid = false;
}

bool VehicleState::fresh(SignalId id, uint32_t now_ms) const {
  const SignalValue& s = get(id);
  return s.valid && elapsed(now_ms, s.updated_ms) <= kSignalStaleMs;
}

float VehicleState::value_or(SignalId id, uint32_t now_ms, float fallback) const {
  return fresh(id, now_ms) ? get(id).value : fallback;
}

bool VehicleState::engine_running(uint32_t now_ms) const {
  return value_or(SignalId::Rpm, now_ms, 0.0f) > kEngineRunningRpm;
}

VehicleState& vehicle() {
  static VehicleState state;
  return state;
}

}  // namespace core
