#include "signal_format.h"

#include <stdio.h>

#include "../core/settings.h"
#include "../core/signals.h"
#include "../core/units.h"
#include "../core/vehicle_state.h"
#include "fonts/ui_icons.h"

namespace ui {

const char* signal_icon(core::SignalId id) {
  using core::SignalId;
  switch (id) {
    case SignalId::Speed: return ICON_SPEED;
    case SignalId::Rpm: return ICON_GAUGE;
    case SignalId::CoolantTemp: return ICON_COOLANT;
    case SignalId::OilTemp: return ICON_OIL_TEMP;
    case SignalId::IntakeTemp: return ICON_THERMOMETER;
    case SignalId::AmbientTemp: return ICON_SUN;
    case SignalId::BatteryVoltage: return ICON_BATTERY;
    case SignalId::FuelLevel: return ICON_FUEL;
    case SignalId::EngineLoad: return ICON_GAUGE_LOW;
    case SignalId::ThrottlePos: return ICON_THROTTLE;
    case SignalId::IntakePressure: return ICON_TURBO;
    case SignalId::Maf: return ICON_AIR;
    case SignalId::TimingAdvance: return ICON_TIMING;
    case SignalId::FuelRate: return ICON_FUEL_RATE;
    case SignalId::ShortFuelTrim:
    case SignalId::LongFuelTrim: return ICON_TRIM;
    case SignalId::RunTime: return ICON_TIMER;
    case SignalId::Gear: return ICON_SHIFT;
    default: return ICON_INFO;
  }
}

SignalText format_signal(core::SignalId id, uint32_t now_ms) {
  const core::Settings& s = core::settings::get();
  const core::VehicleState& vehicle = core::vehicle();
  SignalText out{};
  out.unit = core::units::label(core::signal_info(id).quantity, s);
  out.valid = vehicle.fresh(id, now_ms);
  if (!out.valid) {
    snprintf(out.value, sizeof(out.value), "%s", kNoValue);
    out.severity = core::Severity::Normal;
    return out;
  }
  const float v = vehicle.get(id).value;
  core::units::format_value(id, v, s, out.value, sizeof(out.value));
  out.severity = core::evaluate(id, v, s);
  return out;
}

}  // namespace ui
