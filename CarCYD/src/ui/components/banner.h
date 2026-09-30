/**
 * @file banner.h
 * Notification banner that slides in over the status bar.
 *
 * Used for driver alerts (sticky until tapped) and short confirmations
 * (dismiss automatically).
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

#include "../../core/types.h"

namespace ui::banner {

using TapFn = void (*)(void* user);

void init();
void deinit();

/**
 * Shows the banner, replacing any current content.
 * @param duration_ms 0 = stays until tapped
 */
void show(core::Severity severity, const char* glyph, const char* title, const char* text, uint32_t duration_ms,
          TapFn on_tap, void* user);

void hide();
bool visible();

/** Owner tag of the currently shown banner (e.g. an alert id), -1 for toasts. */
int owner();
void set_owner(int owner);

void tick(uint32_t now_ms);

}  // namespace ui::banner
