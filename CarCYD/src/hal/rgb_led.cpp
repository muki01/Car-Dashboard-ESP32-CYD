#include "rgb_led.h"

#include <Arduino.h>
#include <math.h>

#include "../config/board_config.h"

namespace hal::rgb_led {
namespace {

constexpr uint32_t kPwmFrequency = 5000;
constexpr uint8_t kPwmBits = 8;

Color g_color = colors::kOff;
Pattern g_pattern = Pattern::Solid;
uint16_t g_period = 500;
uint8_t g_intensity = 100;
uint8_t g_written[3] = {255, 255, 255};  // force the first write

void write_channel(uint8_t index, int pin, uint8_t value) {
  if (g_written[index] == value) return;
  g_written[index] = value;
  ledcWrite(pin, value);
}

void write(Color c, float gain) {
  const float k = gain * g_intensity / 100.0f;
  write_channel(0, board::kLedRed, static_cast<uint8_t>(c.r * k));
  write_channel(1, board::kLedGreen, static_cast<uint8_t>(c.g * k));
  write_channel(2, board::kLedBlue, static_cast<uint8_t>(c.b * k));
}

}  // namespace

void init() {
  const int pins[] = {board::kLedRed, board::kLedGreen, board::kLedBlue};
  const uint8_t channels[] = {board::kLedcRed, board::kLedcGreen, board::kLedcBlue};
  for (uint8_t i = 0; i < 3; ++i) {
    ledcAttachChannel(pins[i], kPwmFrequency, kPwmBits, channels[i]);
    ledcOutputInvert(pins[i], true);  // LEDs are active low
  }
  write(colors::kOff, 0.0f);
}

void set(Color color, Pattern pattern, uint16_t period_ms) {
  g_color = color;
  g_pattern = pattern;
  g_period = period_ms > 0 ? period_ms : 1;
}

void off() { set(colors::kOff); }

void set_intensity(uint8_t percent) { g_intensity = percent > 100 ? 100 : percent; }

void update(uint32_t now_ms) {
  float gain = 1.0f;
  switch (g_pattern) {
    case Pattern::Solid:
      break;
    case Pattern::Blink:
      gain = (now_ms % g_period) < g_period / 2 ? 1.0f : 0.0f;
      break;
    case Pattern::Pulse: {
      const float phase = static_cast<float>(now_ms % g_period) / g_period;
      gain = 0.5f - 0.5f * cosf(phase * 2.0f * static_cast<float>(M_PI));
      break;
    }
  }
  write(g_color, gain);
}

}  // namespace hal::rgb_led
