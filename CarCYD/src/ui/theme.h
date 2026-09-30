/**
 * @file theme.h
 * Design tokens of the CarCYD UI: layout grid, colours and typography.
 *
 * Every visual constant lives here; components never hard-code colours or
 * sizes. See docs/ui-guidelines.md for the rationale behind the values.
 */
#pragma once

#include <lvgl.h>

#include "../core/types.h"
#include "fonts/ui_fonts.h"
#include "fonts/ui_icons.h"

namespace ui {

// ---- Layout (pixels, landscape 320 x 240) -------------------------------------------
namespace layout {
constexpr int32_t kScreenW = 320;
constexpr int32_t kScreenH = 240;
constexpr int32_t kRailW = 56;     ///< navigation rail on the driver side
constexpr int32_t kStatusH = 28;   ///< status bar above the content
constexpr int32_t kContentW = kScreenW - kRailW;
constexpr int32_t kContentH = kScreenH - kStatusH;
constexpr int32_t kNavItemH = kScreenH / 5;

constexpr int32_t kPad = 8;         ///< outer padding
constexpr int32_t kGap = 6;         ///< gap between cards
constexpr int32_t kRadius = 10;     ///< cards
constexpr int32_t kRadiusSm = 6;    ///< chips, small buttons
constexpr int32_t kTouchMin = 40;   ///< minimum touch target (resistive panel)
constexpr int32_t kRowH = 44;       ///< list rows
constexpr int32_t kButtonH = 40;
}  // namespace layout

// ---- Colours --------------------------------------------------------------------------
namespace color {
lv_color_t bg();          ///< app background
lv_color_t surface();     ///< rail and status bar
lv_color_t card();        ///< cards, list rows
lv_color_t card_hi();     ///< pressed / elevated
lv_color_t stroke();      ///< dividers, outlines
lv_color_t track();       ///< gauge tracks
lv_color_t text();        ///< primary text
lv_color_t text_dim();    ///< secondary text
lv_color_t text_faint();  ///< disabled text, minor ticks
lv_color_t ok();
lv_color_t info();
lv_color_t warn();
lv_color_t crit();
lv_color_t accent();      ///< user selected accent
lv_color_t on_accent();   ///< text on accent backgrounds
lv_color_t accent_of(core::Accent accent);
/** Colour of a value with the given severity (Normal -> primary text). */
lv_color_t severity(core::Severity severity);
/** Same, but Normal -> accent (used for gauge indicators). */
lv_color_t severity_accent(core::Severity severity);
}  // namespace color

// ---- Typography -------------------------------------------------------------------------
namespace font {
inline const lv_font_t* caption() { return &ui_font_12; }  ///< units, captions
inline const lv_font_t* body() { return &ui_font_14; }     ///< list text
inline const lv_font_t* title() { return &ui_font_16; }    ///< titles, buttons
inline const lv_font_t* value() { return &ui_font_20; }    ///< tile values
inline const lv_font_t* large() { return &ui_font_28; }    ///< hero values
inline const lv_font_t* speed() { return &ui_font_speed; } ///< speedometer digits
inline const lv_font_t* icon24() { return &ui_icons_24; }
inline const lv_font_t* icon48() { return &ui_icons_48; }
}  // namespace font

// ---- Motion -----------------------------------------------------------------------------
namespace motion {
constexpr uint32_t kFast = 150;
constexpr uint32_t kNormal = 250;
}  // namespace motion

/** Applies the LVGL default theme (dark) with the current accent colour. */
void theme_apply(lv_display_t* display);

}  // namespace ui
