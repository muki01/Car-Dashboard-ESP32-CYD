/**
 * @file units.h
 * Conversion from base units to the user's display units, and formatting.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "settings.h"
#include "types.h"

namespace core::units {

/** Converts a base-unit value of `q` to the configured display unit. */
float convert(Quantity q, float base, const Settings& s);

/** Display unit label of `q` (e.g. "km/h", "°C"). Empty for unit-less values. */
const char* label(Quantity q, const Settings& s);

/** Number of decimals used to display `id` in the configured unit. */
uint8_t decimals(SignalId id, const Settings& s);

/** Formats the value of `id` (given in base units) without the unit. */
void format_value(SignalId id, float base, const Settings& s, char* out, size_t len);

/** Formats a number with a fixed number of decimals ("-" sign kept, "-0" avoided). */
void format_number(float value, uint8_t decimals, char* out, size_t len);

/** "1:02:03" or "12:34" */
void format_duration(uint32_t seconds, char* out, size_t len);

/** "1:02" (hours:minutes) */
void format_hours_minutes(uint32_t seconds, char* out, size_t len);

// ---- Trip related ---------------------------------------------------------------

float distance(float km, const Settings& s);
const char* distance_label(const Settings& s);

float volume(float litres, const Settings& s);
const char* volume_label(const Settings& s);

/** Average consumption for `litres` over `km`. Returns false if not meaningful yet. */
bool consumption(float litres, float km, const Settings& s, float& out);
const char* consumption_label(const Settings& s);

/** Target speed of the acceleration timer in km/h (100 km/h or 60 mph). */
float accel_target_kmh(const Settings& s);
const char* accel_label(const Settings& s);

}  // namespace core::units
