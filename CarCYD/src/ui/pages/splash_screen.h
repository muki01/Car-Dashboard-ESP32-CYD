/**
 * @file splash_screen.h
 * Boot screen with product identity and start-up progress.
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace ui::splash {

/** Creates and loads the splash screen. */
lv_obj_t* create();

void set_progress(uint8_t percent, const char* text);

}  // namespace ui::splash
