/**
 * @file buzzer.h
 * Non-blocking tone sequencer for the on-board speaker amplifier.
 *
 * A new sound only interrupts the current one if it has the same or a higher
 * priority, so a touch click never cuts a warning chime short.
 */
#pragma once

#include <stdint.h>

namespace hal::buzzer {

enum class Sound : uint8_t { Click = 0, Confirm, Error, Startup, Warning, Critical };

void init();
void set_volume(uint8_t percent);
void play(Sound sound);
void stop();
void update(uint32_t now_ms);

}  // namespace hal::buzzer
