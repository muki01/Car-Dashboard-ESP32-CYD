#include "units.h"

#include <math.h>
#include <stdio.h>

#include "signals.h"

namespace core::units {
namespace {

constexpr float kKmToMi = 0.621371f;
constexpr float kLitreToUsGallon = 0.264172f;
constexpr float kKpaToPsi = 0.145038f;
constexpr float kL100kmToMpgUs = 235.215f;

bool imperial(const Settings& s) { return s.speed_unit == SpeedUnit::Mph; }

}  // namespace

float convert(Quantity q, float v, const Settings& s) {
  switch (q) {
    case Quantity::Speed: return imperial(s) ? v * kKmToMi : v;
    case Quantity::Temperature: return s.temp_unit == TempUnit::Fahrenheit ? v * 1.8f + 32.0f : v;
    case Quantity::Pressure:
      switch (s.pressure_unit) {
        case PressureUnit::Bar: return v / 100.0f;
        case PressureUnit::Psi: return v * kKpaToPsi;
        default: return v;
      }
    case Quantity::VolumeFlow: return imperial(s) ? v * kLitreToUsGallon : v;
    default: return v;
  }
}

const char* label(Quantity q, const Settings& s) {
  switch (q) {
    case Quantity::Speed: return imperial(s) ? "mph" : "km/h";
    case Quantity::Rpm: return "rpm";
    case Quantity::Temperature: return s.temp_unit == TempUnit::Fahrenheit ? "°F" : "°C";
    case Quantity::Voltage: return "V";
    case Quantity::Percent: return "%";
    case Quantity::Pressure:
      switch (s.pressure_unit) {
        case PressureUnit::Bar: return "bar";
        case PressureUnit::Psi: return "psi";
        default: return "kPa";
      }
    case Quantity::MassFlow: return "g/s";
    case Quantity::Angle: return "°";
    case Quantity::VolumeFlow: return imperial(s) ? "gal/h" : "L/h";
    case Quantity::Duration: return "";
    case Quantity::Gear: return "";
  }
  return "";
}

uint8_t decimals(SignalId id, const Settings& s) {
  switch (id) {
    case SignalId::IntakePressure:
      switch (s.pressure_unit) {
        case PressureUnit::Bar: return 2;
        case PressureUnit::Psi: return 1;
        default: return 0;
      }
    case SignalId::BatteryVoltage:
    case SignalId::Maf:
    case SignalId::TimingAdvance:
    case SignalId::FuelRate:
    case SignalId::ShortFuelTrim:
    case SignalId::LongFuelTrim: return 1;
    default: return 0;
  }
}

void format_number(float value, uint8_t decimals, char* out, size_t len) {
  const float scale = powf(10.0f, decimals);
  float rounded = roundf(value * scale) / scale;
  if (rounded == 0.0f) rounded = 0.0f;  // turns -0 into 0
  snprintf(out, len, "%.*f", decimals, static_cast<double>(rounded));
}

void format_value(SignalId id, float base, const Settings& s, char* out, size_t len) {
  if (id == SignalId::RunTime) {
    format_duration(base > 0 ? static_cast<uint32_t>(base) : 0, out, len);
    return;
  }
  if (id == SignalId::Gear) {
    const int gear = static_cast<int>(lroundf(base));
    if (gear <= 0) {
      snprintf(out, len, "%s", gear < 0 ? "R" : "N");
    } else {
      snprintf(out, len, "%d", gear);
    }
    return;
  }
  format_number(convert(signal_info(id).quantity, base, s), decimals(id, s), out, len);
}

void format_duration(uint32_t seconds, char* out, size_t len) {
  const uint32_t h = seconds / 3600;
  const uint32_t m = (seconds / 60) % 60;
  const uint32_t sec = seconds % 60;
  if (h > 0) {
    snprintf(out, len, "%lu:%02lu:%02lu", static_cast<unsigned long>(h), static_cast<unsigned long>(m),
             static_cast<unsigned long>(sec));
  } else {
    snprintf(out, len, "%lu:%02lu", static_cast<unsigned long>(m), static_cast<unsigned long>(sec));
  }
}

void format_hours_minutes(uint32_t seconds, char* out, size_t len) {
  snprintf(out, len, "%lu:%02lu", static_cast<unsigned long>(seconds / 3600),
           static_cast<unsigned long>((seconds / 60) % 60));
}

float distance(float km, const Settings& s) { return imperial(s) ? km * kKmToMi : km; }
const char* distance_label(const Settings& s) { return imperial(s) ? "mi" : "km"; }

float volume(float litres, const Settings& s) { return imperial(s) ? litres * kLitreToUsGallon : litres; }
const char* volume_label(const Settings& s) { return imperial(s) ? "gal" : "L"; }

bool consumption(float litres, float km, const Settings& s, float& out) {
  if (km < 0.2f || litres <= 0.0f) return false;
  const float l_per_100km = litres / km * 100.0f;
  out = imperial(s) ? kL100kmToMpgUs / l_per_100km : l_per_100km;
  return true;
}

const char* consumption_label(const Settings& s) { return imperial(s) ? "mpg" : "L/100km"; }

float accel_target_kmh(const Settings& s) { return imperial(s) ? 60.0f / kKmToMi : 100.0f; }
const char* accel_label(const Settings& s) { return imperial(s) ? "0-60 mph" : "0-100 km/h"; }

}  // namespace core::units
