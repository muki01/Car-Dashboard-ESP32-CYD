/**
 * @file alerts.h
 * Driver warnings derived from the vehicle state.
 *
 * Every alert is debounced (a condition must persist before it is raised) and
 * uses hysteresis (it clears only after the value is clearly back in range),
 * so noisy signals never make the UI or the buzzer flicker. An alert that is
 * acknowledged by the driver stays silent until it escalates or re-occurs.
 */
#pragma once

#include <stdint.h>

#include "i18n.h"
#include "settings.h"
#include "types.h"
#include "vehicle_state.h"

namespace core {

enum class AlertId : uint8_t {
  CoolantHigh,
  OilHigh,
  BatteryLow,
  BatteryHigh,
  FuelLow,
  Overspeed,
  CheckEngine,
  LinkLost,
  Count
};

constexpr uint8_t kAlertCount = static_cast<uint8_t>(AlertId::Count);

struct Alert {
  Severity severity = Severity::Normal;  ///< Normal = not active
  bool acknowledged = false;
  uint32_t since_ms = 0;
  bool active() const { return severity >= Severity::Warning; }
};

class AlertManager {
 public:
  void update(const VehicleState& state, const Settings& settings, uint32_t now_ms);
  void reset();

  const Alert& get(AlertId id) const { return alerts_[static_cast<uint8_t>(id)]; }

  /**
   * Highest severity of all active alerts (Normal if none). The check engine
   * alert can be excluded where the MIL already has its own indicator.
   */
  Severity highest(bool include_check_engine = true) const;

  /** Most severe active, unacknowledged alert; AlertId::Count if none. */
  AlertId top_unacknowledged() const;

  void acknowledge(AlertId id);

  /** Incremented whenever an alert is raised, escalated, cleared or acknowledged. */
  uint32_t revision() const { return revision_; }

 private:
  struct Pending {
    Severity severity = Severity::Normal;
    uint32_t since_ms = 0;
  };

  void apply(AlertId id, Severity candidate, uint32_t debounce_ms, uint32_t now_ms);

  Alert alerts_[kAlertCount];
  Pending pending_[kAlertCount];
  uint32_t revision_ = 0;
  bool was_connected_ = false;
  uint32_t disconnected_since_ = 0;
};

AlertManager& alerts();

/** Title text of an alert. */
Str alert_title(AlertId id);

/** Signal whose value explains the alert, or SignalId::Count. */
SignalId alert_signal(AlertId id);

}  // namespace core
