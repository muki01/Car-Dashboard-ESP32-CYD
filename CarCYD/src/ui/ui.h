/**
 * @file ui.h
 * Public API of the user interface.
 *
 * Screen structure (landscape 320 x 240):
 *
 *   +------+-------------------------------------+
 *   |      | status bar (title, status icons)    |  28 px
 *   | nav  +-------------------------------------+
 *   | rail |                                     |
 *   |      |        page content 264 x 212       |
 *   | 56px |                                     |
 *   +------+-------------------------------------+
 *
 * Overlays (alert banner, dialogs) live on lv_layer_top(). The splash and the
 * touch calibration are separate screens.
 *
 * All functions must be called from the LVGL (main loop) context.
 */
#pragma once

#include <lvgl.h>

#include "../core/types.h"

namespace ui {

enum class PageId : uint8_t { Dashboard = 0, Live, Diagnostics, Trip, Settings, Count };

/** Initialises the theme. Call once after the display driver is ready. */
void init(lv_display_t* display);

// ---- Boot -------------------------------------------------------------------------------
void splash_show();
void splash_progress(uint8_t percent, const char* text);

/** Builds the main UI and switches to it. Runs the calibration first if requested. */
void start(bool run_touch_calibration);

// ---- Navigation --------------------------------------------------------------------------
void navigate(PageId page);
PageId current_page();

/** Opens the touch calibration screen; returns to the current page afterwards. */
void open_touch_calibration();

/** Rebuilds all widgets (language / accent change). Deferred to a safe point. */
void request_rebuild();

// ---- Shell services for pages ------------------------------------------------------------
void set_title(const char* title);
void show_back_button(bool show);

/** Short, self-dismissing notification. */
void toast(core::Severity severity, const char* title, const char* text = nullptr);

/** Called by the app for hardware button presses. */
void on_hardware_button_short();

}  // namespace ui
