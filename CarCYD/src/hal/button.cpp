#include "button.h"

#include <Arduino.h>

#include "../config/board_config.h"
#include "../core/util.h"

namespace hal::button {
namespace {

constexpr uint32_t kDebounceMs = 30;
constexpr uint32_t kLongPressMs = 1200;

bool g_stable = false;      // debounced state (true = pressed)
bool g_last_raw = false;
uint32_t g_changed_ms = 0;
uint32_t g_pressed_ms = 0;
bool g_long_sent = false;

}  // namespace

void init() {
  pinMode(board::kBootButton, INPUT_PULLUP);
  g_stable = g_last_raw = held();
  g_long_sent = g_stable;  // a button held during boot must not trigger an action
}

bool held() { return digitalRead(board::kBootButton) == LOW; }

Event poll(uint32_t now_ms) {
  const bool raw = held();
  if (raw != g_last_raw) {
    g_last_raw = raw;
    g_changed_ms = now_ms;
  }
  if (core::elapsed(now_ms, g_changed_ms) < kDebounceMs) return Event::None;

  if (raw != g_stable) {
    g_stable = raw;
    if (g_stable) {
      g_pressed_ms = now_ms;
      g_long_sent = false;
      return Event::None;
    }
    // Released.
    const bool was_long = g_long_sent;
    g_long_sent = false;
    return was_long ? Event::None : Event::Short;
  }

  if (g_stable && !g_long_sent && core::elapsed(now_ms, g_pressed_ms) >= kLongPressMs) {
    g_long_sent = true;
    return Event::Long;
  }
  return Event::None;
}

}  // namespace hal::button
