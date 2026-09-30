/**
 * @file util.h
 * Small numeric helpers shared by all layers.
 */
#pragma once

#include <math.h>
#include <stddef.h>
#include <stdint.h>

namespace core {

template <typename T>
constexpr T clamp(T v, T lo, T hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

constexpr float lerp(float a, float b, float t) { return a + (b - a) * t; }

/** Maps `v` from [in_lo, in_hi] to [out_lo, out_hi] with clamping. */
inline float map_clamped(float v, float in_lo, float in_hi, float out_lo, float out_hi) {
  if (in_hi == in_lo) return out_lo;
  const float t = clamp((v - in_lo) / (in_hi - in_lo), 0.0f, 1.0f);
  return lerp(out_lo, out_hi, t);
}

/** First order low-pass filter step. `alpha` in (0, 1], 1 = no filtering. */
inline float ema(float state, float sample, float alpha) { return state + alpha * (sample - state); }

/** Exponential smoothing factor for a time constant `tau_ms` and step `dt_ms`. */
inline float ema_alpha(uint32_t dt_ms, uint32_t tau_ms) {
  if (tau_ms == 0) return 1.0f;
  return 1.0f - expf(-static_cast<float>(dt_ms) / static_cast<float>(tau_ms));
}

/** Elapsed milliseconds, safe across the 32-bit wrap. */
constexpr uint32_t elapsed(uint32_t now, uint32_t since) { return now - since; }

/** CRC-32 (IEEE 802.3, reflected, init/xorout 0xFFFFFFFF). */
uint32_t crc32(const void* data, size_t len);

}  // namespace core
