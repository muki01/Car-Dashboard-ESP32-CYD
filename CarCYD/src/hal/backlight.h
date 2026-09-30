/**
 * @file backlight.h
 * PWM backlight with a perceptual brightness curve and smooth transitions.
 */
#pragma once

#include <stdint.h>

namespace hal::backlight {

void init();

/** Sets the target level (0..100 %). The output ramps smoothly towards it. */
void set_target(uint8_t percent);

/** Jumps to the level immediately (used at start-up). */
void set_now(uint8_t percent);

/** Call periodically (every few tens of ms). */
void update(uint32_t now_ms);

/** Level currently applied (0..100 %). */
uint8_t level();

}  // namespace hal::backlight
