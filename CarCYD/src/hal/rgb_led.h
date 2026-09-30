/**
 * @file rgb_led.h
 * On-board RGB LED used as status / shift light, with simple patterns.
 */
#pragma once

#include <stdint.h>

namespace hal::rgb_led {

struct Color {
  uint8_t r, g, b;
};

namespace colors {
constexpr Color kOff{0, 0, 0};
constexpr Color kRed{255, 0, 0};
constexpr Color kAmber{255, 70, 0};
constexpr Color kGreen{0, 255, 0};
constexpr Color kBlue{0, 60, 255};
}  // namespace colors

enum class Pattern : uint8_t { Solid, Blink, Pulse };

void init();

/** Sets colour + pattern. `period_ms` is used by Blink / Pulse. */
void set(Color color, Pattern pattern = Pattern::Solid, uint16_t period_ms = 500);
void off();

/** Global intensity 0..100 % (follows the backlight at night). */
void set_intensity(uint8_t percent);

void update(uint32_t now_ms);

}  // namespace hal::rgb_led
