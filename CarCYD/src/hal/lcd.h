/**
 * @file lcd.h
 * LovyanGFX device for the ESP32-2432S028R: ILI9341/ST7789 panel on HSPI with
 * DMA, XPT2046 touch on a software SPI bus (keeps VSPI free for the SD card).
 *
 * Panel/touch geometry follows the LovyanGFX board definition for the Sunton
 * ESP32-2432S028 ("CYD"). Only display.cpp and touch.cpp use this header.
 */
#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

namespace hal {

/** The display + touch device (initialised by display::init()). */
lgfx::LGFX_Device& lcd();

/** Human readable panel description, e.g. "ILI9341 320x240". */
const char* lcd_panel_name();

/**
 * Identifies the configured panel type and display/touch orientation. A touch
 * calibration is only valid for the orientation it was made with.
 */
uint32_t lcd_orientation_id();

}  // namespace hal
