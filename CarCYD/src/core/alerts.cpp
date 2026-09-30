#include "alerts.h"

#include <math.h>

#include "log.h"
#include "signals.h"
#include "util.h"

namespace core {
namespace {

constexpr uint32_t kLinkLostDelayMs = 3000;

/** Severity for "too high" limits with hysteresis relative to the current state. */
Severity high_side(float v, float warn, float crit, float hysteresis, Severity current) {
  if (!isnan(crit) && (v >= crit || (current == Severity::Critical && v > crit - hysteresis))) {
    return Severity::Critical;
  }
  if (!isnan(warn) && (v >= warn || (current >= Severity::Warning && v > warn - hysteresis))) {
    return Severity::Warning;
  }
  return Severity::Normal;
}

/** Severity for "too low" limits with hysteresis relative to the current state. */
Severity low_side(float v, float warn, float crit, float hysteresis, Severity current) {
  if (!isnan(crit) && (v <= crit || (current == Severity::Critical && v < crit + hysteresis))) {
    return Severity::Critical;
  }
  if (!isnan(warn) && (v <= warn || (current >= Severity::Warning && v < warn + hysteresis))) {
    return Severity::Warning;
  }
  return Severity::Normal;
}

}  // namespace

void AlertManager::reset() {
  for (uint8_t i = 0; i < kAlertCount; ++i) {
    alerts_[i] = Alert{};
    pending_[i] = Pending{};
  }
  was_connected_ = false;
  revision_++;
}

void AlertManager::apply(AlertId id, Severity candidate, uint32_t debounce_ms, uint32_t now_ms) {
  const uint8_t i = static_cast<uint8_t>(id);
  Alert& alert = alerts_[i];
  Pending& pending = pending_[i];

  if (candidate == alert.severity) {
    pending.severity = candidate;
    return;
  }

  if (candidate < alert.severity) {
    // De-escalation happens immediately; hysteresis already filtered the value.
    alert.severity = candidate;
    if (!alert.active()) alert.acknowledged = false;
    pending.severity = candidate;
    revision_++;
    return;
  }

  // Escalation must persist for the debounce time.
  if (pending.severity != candidate) {
    pending.severity = candidate;
    pending.since_ms = now_ms;
  }
  if (elapsed(now_ms, pending.since_ms) < debounce_ms) return;

  alert.severity = candidate;
  alert.acknowledged = false;
  alert.since_ms = now_ms;
  revision_++;
  LOG_I("ALRT", "alert %u raised (severity %u)", static_cast<unsigned>(i), static_cast<unsigned>(candidate));
}

void AlertManager::update(const VehicleState& state, const Settings& s, uint32_t now) {
  auto current = [this](AlertId id) { return get(id).severity; };

  // ---- Threshold alerts ----------------------------------------------------------
  auto evaluate_high = [&](AlertId id, SignalId sig, float hysteresis, uint32_t debounce) {
    Severity candidate = Severity::Normal;
    if (state.fresh(sig, now)) {
      const Limits l = limits(sig, s);
      candidate = high_side(state.get(sig).value, l.warn_high, l.crit_high, hysteresis, current(id));
    }
    apply(id, candidate, debounce, now);
  };
  auto evaluate_low = [&](AlertId id, SignalId sig, float hysteresis, uint32_t debounce) {
    Severity candidate = Severity::Normal;
    if (state.fresh(sig, now)) {
      const Limits l = limits(sig, s);
      candidate = low_side(state.get(sig).value, l.warn_low, l.crit_low, hysteresis, current(id));
    }
    apply(id, candidate, debounce, now);
  };

  evaluate_high(AlertId::CoolantHigh, SignalId::CoolantTemp, 3.0f, 2000);
  evaluate_high(AlertId::OilHigh, SignalId::OilTemp, 3.0f, 2000);
  evaluate_high(AlertId::BatteryHigh, SignalId::BatteryVoltage, 0.3f, 3000);
  evaluate_high(AlertId::Overspeed, SignalId::Speed, 3.0f, 1000);
  evaluate_low(AlertId::FuelLow, SignalId::FuelLevel, 2.0f, 10000);

  // Low voltage is expected while cranking; only judge a stable system.
  const float rpm = state.value_or(SignalId::Rpm, now, 0.0f);
  const bool cranking = rpm > 50.0f && rpm < kEngineRunningRpm;
  if (!cranking) evaluate_low(AlertId::BatteryLow, SignalId::BatteryVoltage, 0.3f, 5000);

  // ---- Check engine light --------------------------------------------------------
  const bool mil = state.dtc.mil_known && state.dtc.mil_on;
  apply(AlertId::CheckEngine, mil ? Severity::Warning : Severity::Normal, 0, now);

  // ---- Connection ------------------------------------------------------------------
  if (state.link_state == LinkState::Connected) {
    was_connected_ = true;
    disconnected_since_ = now;
  }
  const bool lost = was_connected_ && state.link_state != LinkState::Connected &&
                    elapsed(now, disconnected_since_) >= kLinkLostDelayMs;
  apply(AlertId::LinkLost, lost ? Severity::Warning : Severity::Normal, 0, now);
}

Severity AlertManager::highest(bool include_check_engine) const {
  Severity best = Severity::Normal;
  for (uint8_t i = 0; i < kAlertCount; ++i) {
    if (!include_check_engine && static_cast<AlertId>(i) == AlertId::CheckEngine) continue;
    const Alert& a = alerts_[i];
    if (a.active() && a.severity > best) best = a.severity;
  }
  return best;
}

AlertId AlertManager::top_unacknowledged() const {
  AlertId best = AlertId::Count;
  Severity best_severity = Severity::Normal;
  for (uint8_t i = 0; i < kAlertCount; ++i) {
    const Alert& a = alerts_[i];
    if (a.active() && !a.acknowledged && a.severity > best_severity) {
      best = static_cast<AlertId>(i);
      best_severity = a.severity;
    }
  }
  return best;
}

void AlertManager::acknowledge(AlertId id) {
  if (id >= AlertId::Count) return;
  Alert& a = alerts_[static_cast<uint8_t>(id)];
  if (a.active() && !a.acknowledged) {
    a.acknowledged = true;
    revision_++;
  }
}

AlertManager& alerts() {
  static AlertManager manager;
  return manager;
}

Str alert_title(AlertId id) {
  switch (id) {
    case AlertId::CoolantHigh: return Str::ALERT_COOLANT;
    case AlertId::OilHigh: return Str::ALERT_OIL;
    case AlertId::BatteryLow: return Str::ALERT_BATTERY_LOW;
    case AlertId::BatteryHigh: return Str::ALERT_BATTERY_HIGH;
    case AlertId::FuelLow: return Str::ALERT_FUEL;
    case AlertId::Overspeed: return Str::ALERT_OVERSPEED;
    case AlertId::CheckEngine: return Str::ALERT_MIL;
    case AlertId::LinkLost:
    default: return Str::ALERT_LINK;
  }
}

SignalId alert_signal(AlertId id) {
  switch (id) {
    case AlertId::CoolantHigh: return SignalId::CoolantTemp;
    case AlertId::OilHigh: return SignalId::OilTemp;
    case AlertId::BatteryLow:
    case AlertId::BatteryHigh: return SignalId::BatteryVoltage;
    case AlertId::FuelLow: return SignalId::FuelLevel;
    case AlertId::Overspeed: return SignalId::Speed;
    default: return SignalId::Count;
  }
}

}  // namespace core
