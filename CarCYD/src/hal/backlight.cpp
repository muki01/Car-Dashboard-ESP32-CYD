#include "backlight.h"

#include <Arduino.h>
#include <math.h>

#include "../config/board_config.h"
#include "../core/util.h"

namespace hal::backlight {
namespace {

constexpr uint32_t kPwmFrequency = 12000;  // above the audible range of the amplifier
constexpr uint8_t kPwmBits = 8;
constexpr float kRampPercentPerSecond = 120.0f;

float g_level = 0.0f;
uint8_t g_target = 0;
uint32_t g_last_ms = 0;

/** Perceived brightness is roughly quadratic in duty cycle. */
uint32_t duty_for(float percent) {
  if (percent <= 0.0f) return 0;
  const float t = percent / 100.0f;
  return static_cast<uint32_t>(2.0f + 253.0f * t * t);
}

void apply() { ledcWrite(board::kLcdBacklight, duty_for(g_level)); }

}  // namespace

void init() {
  ledcAttachChannel(board::kLcdBacklight, kPwmFrequency, kPwmBits, board::kLedcBacklight);
  set_now(0);
}

void set_target(uint8_t percent) { g_target = core::clamp<uint8_t>(percent, 0, 100); }

void set_now(uint8_t percent) {
  g_target = core::clamp<uint8_t>(percent, 0, 100);
  g_level = g_target;
  apply();
}

void update(uint32_t now_ms) {
  const uint32_t dt = g_last_ms ? core::elapsed(now_ms, g_last_ms) : 0;
  g_last_ms = now_ms;
  if (static_cast<uint8_t>(g_level + 0.5f) == g_target) return;

  const float step = kRampPercentPerSecond * dt / 1000.0f;
  if (g_level < g_target) {
    g_level = fminf(g_level + step, g_target);
  } else {
    g_level = fmaxf(g_level - step, g_target);
  }
  apply();
}

uint8_t level() { return static_cast<uint8_t>(g_level + 0.5f); }

}  // namespace hal::backlight
