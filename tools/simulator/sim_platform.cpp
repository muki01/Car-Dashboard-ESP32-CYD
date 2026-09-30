/**
 * @file sim_platform.cpp
 * core/platform.h implemented for the desktop simulator.
 */
#include <stdio.h>
#include <stdlib.h>

#include "../../CarCYD/src/core/platform.h"
#include "sim.h"

namespace platform {

uint32_t millis() { return sim::now(); }

void system_info(SystemInfo& out) {
  out.chip_model = "ESP32-D0WD-V3";
  out.chip_revision = 3;
  out.cores = 2;
  out.cpu_mhz = 240;
  out.flash_bytes = 4 * 1024 * 1024;
  out.heap_free = 187 * 1024;
  out.heap_min_free = 171 * 1024;
  out.heap_total = 320 * 1024;
  out.sdk_version = "sim";
  out.board_name = "ESP32-2432S028R";
  out.panel_name = "ILI9341 320x240";
}

void restart() {
  printf("[sim] restart requested\n");
  exit(0);
}

void factory_reset() {
  printf("[sim] factory reset requested\n");
  exit(0);
}

uint16_t light_sensor_raw() { return 412; }
uint8_t ambient_level() { return 64; }
uint8_t backlight_level() { return 85; }

void play(Sound) {}

bool touch_read_raw(TouchSample& out) {
  out.pressed = sim::touch_pressed();
  out.raw_x = static_cast<int16_t>(sim::touch_x());
  out.raw_y = static_cast<int16_t>(sim::touch_y());
  out.pressure = out.pressed ? 1 : 0;
  return true;
}

void touch_apply_calibration(const core::TouchCalibration& cal) {
  printf("[sim] calibration a=%.3f b=%.3f c=%.1f d=%.3f e=%.3f f=%.1f\n", cal.a, cal.b, cal.c, cal.d, cal.e, cal.f);
}

void touch_input_enable(bool enable) { sim::set_touch_input(enable); }

}  // namespace platform
