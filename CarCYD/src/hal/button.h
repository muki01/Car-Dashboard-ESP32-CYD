/**
 * @file button.h
 * Debounced BOOT button with short / long press detection.
 *
 *   short press : acknowledge alert, else next page
 *   long press  : back to the dashboard
 *   held at power-up : touch calibration
 */
#pragma once

#include <stdint.h>

namespace hal::button {

enum class Event : uint8_t { None, Short, Long };

void init();

/** Raw (debounce-free) state, used right after reset. */
bool held();

Event poll(uint32_t now_ms);

}  // namespace hal::button
