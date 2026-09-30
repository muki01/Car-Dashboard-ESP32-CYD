/**
 * @file signals.h
 * Static metadata of the vehicle signals and their limits.
 *
 * All values are handled in base units (see Quantity); conversion to the
 * user's units happens only for presentation (units.h).
 */
#pragma once

#include <math.h>

#include "i18n.h"
#include "settings.h"
#include "types.h"

namespace core {

struct SignalInfo {
  SignalId id;
  Str name;        ///< full name (Live Data)
  Str short_name;  ///< tile caption
  Quantity quantity;
  float min;  ///< nominal display range (base units)
  float max;
};

const SignalInfo& signal_info(SignalId id);

/** Nominal range for gauges; RPM follows the configured tachometer range. */
void signal_range(SignalId id, const Settings& settings, float& min, float& max);

/**
 * Warning limits of a signal (base units, NAN = not used). This is the single
 * source of truth for value colouring (evaluate) and the alert manager.
 */
struct Limits {
  float warn_high = NAN;
  float crit_high = NAN;
  float warn_low = NAN;
  float crit_low = NAN;
  float info_below = NAN;  ///< e.g. engine still cold
};

Limits limits(SignalId id, const Settings& settings);

/** Severity of a value without hysteresis (UI colouring). */
Severity evaluate(SignalId id, float value, const Settings& settings);

/** Engine is considered running above this speed. */
constexpr float kEngineRunningRpm = 400.0f;

}  // namespace core
