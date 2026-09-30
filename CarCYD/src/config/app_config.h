/**
 * @file app_config.h
 * Firmware identity and application tunables.
 */
#pragma once

// ---- Identity -----------------------------------------------------------------------
#define APP_NAME "CarCYD"
#define APP_VERSION_MAJOR 1
#define APP_VERSION_MINOR 1
#define APP_VERSION_PATCH 0
#define APP_VERSION "1.1.0"

// ---- Logging --------------------------------------------------------------------------
// 0 = errors, 1 = warnings, 2 = info, 3 = debug
#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL 2
#endif
#define APP_SERIAL_BAUD 115200

// ---- Timing (ms) --------------------------------------------------------------------------
#define APP_UI_TICK_MS 40            // UI refresh (25 Hz)
#define APP_BACKLIGHT_TICK_MS 50     // backlight / ambient light filter
#define APP_TRIP_SAVE_PERIOD_MS 60000 // trip persistence while driving
#define APP_SPLASH_MIN_MS 1200       // minimum splash duration
#define APP_ALERT_REPEAT_MS 15000    // critical alert sound repetition

// ---- Display ---------------------------------------------------------------------------------
#define APP_DRAW_BUFFER_LINES 40     // 2 x (320 x 40 x 2 B) = 50 KB DMA buffers
