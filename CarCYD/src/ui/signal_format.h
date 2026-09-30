/**
 * @file signal_format.h
 * Presentation helpers for vehicle signals (icon, value text, unit, colour).
 */
#pragma once

#include <lvgl.h>
#include <stddef.h>
#include <stdint.h>

#include "../core/types.h"

namespace ui {

/** Icon glyph (ui_icons.h) representing a signal. */
const char* signal_icon(core::SignalId id);

struct SignalText {
  char value[16];
  const char* unit;
  core::Severity severity;
  bool valid;
};

/** Formats the current value of `id` from the vehicle state ("--" when stale). */
SignalText format_signal(core::SignalId id, uint32_t now_ms);

/** Placeholder shown for missing values. */
constexpr const char* kNoValue = "--";

}  // namespace ui
