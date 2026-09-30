/**
 * @file light_sensor.h
 * Ambient light from the on-board LDR, heavily filtered so that passing
 * shadows (street lights, trees) do not make the backlight pump.
 */
#pragma once

#include <stdint.h>

namespace hal::light_sensor {

void init();

/** Call periodically; samples and filters the sensor. */
void update(uint32_t now_ms);

/** Filtered ADC reading. */
uint16_t raw();

/** Ambient level 0 (dark) .. 100 (bright daylight). */
uint8_t level();

}  // namespace hal::light_sensor
