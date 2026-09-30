/**
 * @file board_config.h
 * Hardware description of the ESP32-2432S028R ("Cheap Yellow Display").
 *
 *   MCU      ESP32-WROOM-32 (2 x 240 MHz, 520 KB SRAM, 4 MB flash, no PSRAM)
 *   Display  2.8" 320x240 TFT, ILI9341 (ST7789 on some 2-USB revisions), HSPI
 *   Touch    XPT2046 resistive controller on its own pins
 *   Extras   RGB LED, light sensor (LDR), speaker amplifier, micro SD, BOOT key
 *
 * See docs/hardware.md for connectors and board variants.
 */
#pragma once

#include <stdint.h>

// ---- Board variant ---------------------------------------------------------------------
// Select the display controller of your board (Arduino IDE: edit here).
//   CYD_PANEL_ILI9341    original board, one micro-USB port  -> CYD_PANEL_INVERT 0
//   CYD_PANEL_ILI9341_2  "CYD2USB" (micro-USB + USB-C)       -> CYD_PANEL_INVERT 1
//   CYD_PANEL_ST7789     2-USB boards fitted with an ST7789 controller -> CYD_PANEL_INVERT 0
// This repository is configured for the 2-USB board below; see docs/hardware.md.
#define CYD_PANEL_ILI9341 1
#define CYD_PANEL_ILI9341_2 2
#define CYD_PANEL_ST7789 3

#ifndef CYD_PANEL
#define CYD_PANEL CYD_PANEL_ILI9341_2
#endif

// Colour inversion: set to 1 only if colours look like a photo negative.
#ifndef CYD_PANEL_INVERT
#define CYD_PANEL_INVERT 1
#endif

// Picture orientation (advanced). Each panel variant above already selects the
// correct value (see src/hal/lcd.cpp); override it only if the picture is still
// mirrored or upside down. LovyanGFX rotation offset: 0-3 = 90 degree steps,
// 4-7 = the same steps mirrored. Touch input follows automatically and the touch
// calibration is requested again after a change.
// #define CYD_PANEL_ROTATION_OFFSET 4

// SPI write clock of the display. 40 MHz is safe on every board seen so far;
// many boards also run at 55 MHz.
#ifndef CYD_LCD_SPI_HZ
#define CYD_LCD_SPI_HZ 40000000
#endif

#define CYD_BOARD_NAME "ESP32-2432S028R"

namespace board {

// ---- Display (HSPI / SPI2, native IO_MUX pins) --------------------------------------------
constexpr int kLcdSclk = 14;
constexpr int kLcdMosi = 13;
constexpr int kLcdMiso = 12;
constexpr int kLcdCs = 15;
constexpr int kLcdDc = 2;
constexpr int kLcdRst = -1;  // tied to EN
constexpr int kLcdBacklight = 21;
constexpr int kLcdWidth = 240;  // native portrait resolution
constexpr int kLcdHeight = 320;

// ---- Touch controller XPT2046 --------------------------------------------------------------
// Driven by a bit-banged SPI so that the VSPI peripheral stays free for the SD card.
constexpr int kTouchClk = 25;
constexpr int kTouchMosi = 32;
constexpr int kTouchMiso = 39;  // input only
constexpr int kTouchCs = 33;
constexpr int kTouchIrq = 36;   // input only, low while touched

// ---- RGB LED (common anode: LOW = on) --------------------------------------------------------
constexpr int kLedRed = 4;
constexpr int kLedGreen = 16;
constexpr int kLedBlue = 17;

// ---- Light sensor (LDR to GND, 1 MOhm pull-up: bright = low reading) ---------------------------
constexpr int kLightSensor = 34;  // ADC1_CH6, input only
// Raw ADC readings (0 dB attenuation) for bright daylight and a dark cabin.
// Check Settings > Display > Ambient light on your board and adjust if needed.
constexpr uint16_t kLightRawBright = 40;
constexpr uint16_t kLightRawDark = 1500;

// ---- Speaker amplifier input (JST "SPEAK" connector) --------------------------------------------
constexpr int kSpeaker = 26;

// ---- Micro SD card (VSPI / SPI3) - reserved for data logging ---------------------------------------
constexpr int kSdSck = 18;
constexpr int kSdMiso = 19;
constexpr int kSdMosi = 23;
constexpr int kSdCs = 5;

// ---- BOOT button -------------------------------------------------------------------------------------
constexpr int kBootButton = 0;  // LOW = pressed

// ---- Expansion connector CN1 (3.3 V, GND, IO22, IO27) --------------------------------------------------
// Reserved for the UART link to the vehicle interface ESP32.
constexpr int kLinkRx = 22;
constexpr int kLinkTx = 27;

// ---- LEDC channel plan -----------------------------------------------------------------------------------
// Channels that share frequency/resolution share a hardware timer. The speaker
// uses a unique configuration so tone changes never affect the backlight/LED.
constexpr uint8_t kLedcBacklight = 0;  // 12 kHz, 8 bit
constexpr uint8_t kLedcSpeaker = 2;    // variable frequency, 10 bit
constexpr uint8_t kLedcRed = 4;        // 5 kHz, 8 bit (shared timer)
constexpr uint8_t kLedcGreen = 5;
constexpr uint8_t kLedcBlue = 6;

}  // namespace board
