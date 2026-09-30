/**
 * @file widgets.h
 * Factory functions for the basic building blocks of the UI.
 *
 * Every widget created here already carries the design tokens (theme.h), so
 * pages only describe structure. Interactive widgets get touch feedback
 * (pressed state + click sound) automatically.
 */
#pragma once

#include <lvgl.h>

#include "theme.h"

namespace ui {

/**
 * Style selector "part in state". C++20 deprecates `|` between different enum
 * types (LV_PART_* | LV_STATE_*), so the operands are combined as integers.
 */
constexpr lv_style_selector_t selector(lv_part_t part, lv_state_t state) {
  return static_cast<lv_style_selector_t>(part) | static_cast<lv_style_selector_t>(state);
}

/** Plain layout container: transparent, no padding, not scrollable, not clickable. */
lv_obj_t* box(lv_obj_t* parent);

/** Same as box() with a flex flow already set. */
lv_obj_t* row(lv_obj_t* parent, int32_t gap = layout::kGap);
lv_obj_t* column(lv_obj_t* parent, int32_t gap = layout::kGap);

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text = "");

/**
 * Makes a label single-line: text that does not fit ends with "...".
 * (LVGL only truncates labels whose height is fixed, so the height is set to
 * one line of the label's font.)
 */
void single_line(lv_obj_t* label);

/** Icon glyph from ui_icons.h rendered with the given (icon capable) font. */
lv_obj_t* icon(lv_obj_t* parent, const char* glyph, const lv_font_t* font, lv_color_t color);

/** Rounded card surface with default padding. */
lv_obj_t* card(lv_obj_t* parent);

enum class ButtonStyle : uint8_t { Primary, Secondary, Danger, Ghost };

/** Text button (optionally prefixed with an icon glyph). Height = layout::kButtonH. */
lv_obj_t* button(lv_obj_t* parent, const char* glyph, const char* text, ButtonStyle style,
                 lv_event_cb_t on_click, void* user);

/** Square icon-only button with an enlarged touch area. */
lv_obj_t* icon_button(lv_obj_t* parent, const char* glyph, int32_t size, lv_event_cb_t on_click, void* user);

/** Small status pill. */
lv_obj_t* chip(lv_obj_t* parent, const char* text, lv_color_t color);
void chip_set(lv_obj_t* chip, const char* text, lv_color_t color);

/** Horizontal 1 px divider. */
lv_obj_t* divider(lv_obj_t* parent);

/** Adds pressed feedback (click sound) to any clickable object. */
void add_touch_feedback(lv_obj_t* obj);

/** Updates a label only if the text actually changed (avoids redraws). */
void set_text(lv_obj_t* label, const char* text);

/** Updates the text colour only if it changed. */
void set_text_color(lv_obj_t* obj, lv_color_t color);

/** Shows or hides an object. */
void set_visible(lv_obj_t* obj, bool visible);

}  // namespace ui
