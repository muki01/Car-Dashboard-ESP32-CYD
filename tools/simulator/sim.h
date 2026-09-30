/**
 * @file sim.h
 * Headless LVGL simulator: virtual clock, scripted touch input, screenshots.
 */
#pragma once

#include <stdint.h>

namespace sim {

uint32_t now();
bool touch_pressed();
int32_t touch_x();
int32_t touch_y();
void set_touch_input(bool enabled);

}  // namespace sim
