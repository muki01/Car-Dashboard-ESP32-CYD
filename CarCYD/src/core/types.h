/**
 * @file types.h
 * Shared value types used across the core, UI and HAL layers.
 */
#pragma once

#include <stdint.h>

namespace core {

/** Vehicle signals exposed by the data link. Order defines the Live Data list. */
enum class SignalId : uint8_t {
  Speed,
  Rpm,
  CoolantTemp,
  OilTemp,
  IntakeTemp,
  AmbientTemp,
  BatteryVoltage,
  FuelLevel,
  EngineLoad,
  ThrottlePos,
  IntakePressure,
  Maf,
  TimingAdvance,
  FuelRate,
  ShortFuelTrim,
  LongFuelTrim,
  RunTime,
  Gear,
  Count
};

constexpr uint8_t kSignalCount = static_cast<uint8_t>(SignalId::Count);

/** Physical quantity of a signal; drives unit conversion and formatting. */
enum class Quantity : uint8_t {
  Speed,        // base: km/h
  Rpm,          // base: 1/min
  Temperature,  // base: degC
  Voltage,      // base: V
  Percent,      // base: %
  Pressure,     // base: kPa (absolute)
  MassFlow,     // base: g/s
  Angle,        // base: deg
  VolumeFlow,   // base: L/h
  Duration,     // base: s
  Gear,         // base: gear number
};

/** Severity of a value or an alert. `Info` is also used for "cold"/"low" hints. */
enum class Severity : uint8_t { Normal = 0, Info, Warning, Critical };

enum class Language : uint8_t { English = 0, Turkish, Count };
enum class SpeedUnit : uint8_t { Kmh = 0, Mph, Count };
enum class TempUnit : uint8_t { Celsius = 0, Fahrenheit, Count };
enum class PressureUnit : uint8_t { Kpa = 0, Bar, Psi, Count };
enum class Accent : uint8_t { Cyan = 0, Amber, Red, Green, Violet, Count };

/** State of the connection to the vehicle interface. */
enum class LinkState : uint8_t { Disconnected = 0, Connecting, Connected, Error };

}  // namespace core
