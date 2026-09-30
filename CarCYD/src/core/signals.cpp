#include "signals.h"

namespace core {
namespace {

// clang-format off
const SignalInfo kSignals[kSignalCount] = {
  {SignalId::Speed,          Str::SIG_SPEED,        Str::SHORT_SPEED,        Quantity::Speed,         0,  240},
  {SignalId::Rpm,            Str::SIG_RPM,          Str::SHORT_RPM,          Quantity::Rpm,           0, 8000},
  {SignalId::CoolantTemp,    Str::SIG_COOLANT,      Str::SHORT_COOLANT,      Quantity::Temperature,  40,  130},
  {SignalId::OilTemp,        Str::SIG_OIL_TEMP,     Str::SHORT_OIL_TEMP,     Quantity::Temperature,  40,  160},
  {SignalId::IntakeTemp,     Str::SIG_INTAKE_TEMP,  Str::SHORT_INTAKE_TEMP,  Quantity::Temperature, -20,   80},
  {SignalId::AmbientTemp,    Str::SIG_AMBIENT_TEMP, Str::SHORT_AMBIENT_TEMP, Quantity::Temperature, -20,   50},
  {SignalId::BatteryVoltage, Str::SIG_BATTERY,      Str::SHORT_BATTERY,      Quantity::Voltage,      10,   16},
  {SignalId::FuelLevel,      Str::SIG_FUEL_LEVEL,   Str::SHORT_FUEL_LEVEL,   Quantity::Percent,       0,  100},
  {SignalId::EngineLoad,     Str::SIG_LOAD,         Str::SHORT_LOAD,         Quantity::Percent,       0,  100},
  {SignalId::ThrottlePos,    Str::SIG_THROTTLE,     Str::SHORT_THROTTLE,     Quantity::Percent,       0,  100},
  {SignalId::IntakePressure, Str::SIG_MAP,          Str::SHORT_MAP,          Quantity::Pressure,     20,  200},
  {SignalId::Maf,            Str::SIG_MAF,          Str::SHORT_MAF,          Quantity::MassFlow,      0,  150},
  {SignalId::TimingAdvance,  Str::SIG_TIMING,       Str::SHORT_TIMING,       Quantity::Angle,       -10,   50},
  {SignalId::FuelRate,       Str::SIG_FUEL_RATE,    Str::SHORT_FUEL_RATE,    Quantity::VolumeFlow,    0,   30},
  {SignalId::ShortFuelTrim,  Str::SIG_STFT,         Str::SHORT_STFT,         Quantity::Percent,     -25,   25},
  {SignalId::LongFuelTrim,   Str::SIG_LTFT,         Str::SHORT_LTFT,         Quantity::Percent,     -25,   25},
  {SignalId::RunTime,        Str::SIG_RUN_TIME,     Str::SHORT_RUN_TIME,     Quantity::Duration,      0, 3600},
  {SignalId::Gear,           Str::SIG_GEAR,         Str::SHORT_GEAR,         Quantity::Gear,          0,    6},
};
// clang-format on

// Fixed engineering limits (base units).
constexpr float kCoolantColdC = 50.0f;
constexpr float kCoolantCriticalMarginC = 10.0f;
constexpr float kOilWarnC = 130.0f;
constexpr float kOilCriticalC = 145.0f;
constexpr float kIntakeWarnC = 65.0f;
constexpr float kBatteryCriticalMarginV = 0.8f;
constexpr float kChargingWarnV = 15.2f;
constexpr float kChargingCriticalV = 16.0f;
constexpr float kFuelTrimWarnPct = 20.0f;

}  // namespace

const SignalInfo& signal_info(SignalId id) {
  const auto index = static_cast<uint8_t>(id);
  return kSignals[index < kSignalCount ? index : 0];
}

void signal_range(SignalId id, const Settings& settings, float& min, float& max) {
  const SignalInfo& info = signal_info(id);
  min = info.min;
  max = (id == SignalId::Rpm) ? static_cast<float>(settings.rpm_max) : info.max;
}

Limits limits(SignalId id, const Settings& s) {
  Limits l;
  switch (id) {
    case SignalId::Speed:
      if (s.overspeed_kmh != 0) l.warn_high = s.overspeed_kmh;
      break;
    case SignalId::Rpm:
      if (s.shift_light) l.warn_high = s.shift_rpm;
      l.crit_high = s.redline_rpm;
      break;
    case SignalId::CoolantTemp:
      l.warn_high = s.coolant_warn_c;
      l.crit_high = s.coolant_warn_c + kCoolantCriticalMarginC;
      l.info_below = kCoolantColdC;
      break;
    case SignalId::OilTemp:
      l.warn_high = kOilWarnC;
      l.crit_high = kOilCriticalC;
      break;
    case SignalId::IntakeTemp:
      l.warn_high = kIntakeWarnC;
      break;
    case SignalId::BatteryVoltage:
      l.warn_low = s.low_voltage_dv / 10.0f;
      l.crit_low = l.warn_low - kBatteryCriticalMarginV;
      l.warn_high = kChargingWarnV;
      l.crit_high = kChargingCriticalV;
      break;
    case SignalId::FuelLevel:
      if (s.fuel_warn_pct != 0) {
        l.warn_low = s.fuel_warn_pct;
        l.crit_low = s.fuel_warn_pct / 2.0f;
      }
      break;
    case SignalId::ShortFuelTrim:
    case SignalId::LongFuelTrim:
      l.warn_high = kFuelTrimWarnPct;
      l.warn_low = -kFuelTrimWarnPct;
      break;
    default:
      break;
  }
  return l;
}

namespace {
bool at_or_above(float v, float limit) { return !isnan(limit) && v >= limit; }
bool at_or_below(float v, float limit) { return !isnan(limit) && v <= limit; }
}  // namespace

Severity evaluate(SignalId id, float v, const Settings& s) {
  const Limits l = limits(id, s);
  if (at_or_above(v, l.crit_high) || at_or_below(v, l.crit_low)) return Severity::Critical;
  if (at_or_above(v, l.warn_high) || at_or_below(v, l.warn_low)) return Severity::Warning;
  if (!isnan(l.info_below) && v < l.info_below) return Severity::Info;
  return Severity::Normal;
}

}  // namespace core
