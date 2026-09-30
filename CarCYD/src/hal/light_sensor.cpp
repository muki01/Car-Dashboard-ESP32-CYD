#include "light_sensor.h"

#include <Arduino.h>
#include <math.h>

#include "../config/board_config.h"
#include "../core/util.h"

namespace hal::light_sensor {
namespace {

constexpr uint32_t kSamplePeriodMs = 100;
constexpr uint32_t kTimeConstantMs = 2500;
constexpr uint8_t kOversample = 4;

float g_filtered = -1.0f;
uint32_t g_last_ms = 0;

uint16_t sample() {
  uint32_t sum = 0;
  for (uint8_t i = 0; i < kOversample; ++i) sum += analogRead(board::kLightSensor);
  return static_cast<uint16_t>(sum / kOversample);
}

}  // namespace

void init() {
  pinMode(board::kLightSensor, INPUT);
  // 0 dB attenuation: the LDR divider only swings a few hundred millivolts.
  analogSetPinAttenuation(board::kLightSensor, ADC_0db);
  g_filtered = sample();
}

void update(uint32_t now_ms) {
  if (g_last_ms != 0 && core::elapsed(now_ms, g_last_ms) < kSamplePeriodMs) return;
  const uint32_t dt = g_last_ms ? core::elapsed(now_ms, g_last_ms) : kSamplePeriodMs;
  g_last_ms = now_ms;
  g_filtered = core::ema(g_filtered, sample(), core::ema_alpha(dt, kTimeConstantMs));
}

uint16_t raw() { return static_cast<uint16_t>(g_filtered < 0 ? 0 : g_filtered + 0.5f); }

uint8_t level() {
  // The LDR resistance is roughly logarithmic in lux: map on a log scale.
  const float lo = logf(static_cast<float>(board::kLightRawBright) + 1.0f);
  const float hi = logf(static_cast<float>(board::kLightRawDark) + 1.0f);
  const float v = logf(fmaxf(g_filtered, 0.0f) + 1.0f);
  const float darkness = core::map_clamped(v, lo, hi, 0.0f, 1.0f);
  return static_cast<uint8_t>((1.0f - darkness) * 100.0f + 0.5f);
}

}  // namespace hal::light_sensor
