/**
 * @file setting_rows.h
 * List rows for settings-style screens. All rows share the same geometry and
 * provide touch targets of at least 40 px for the resistive panel.
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace ui::rows {

/** Vertical, scrollable list filling `parent`. */
lv_obj_t* list(lv_obj_t* parent);

/** Small caption above a group of rows. */
lv_obj_t* section(lv_obj_t* list, const char* text);

/** Row with icon, title, value and chevron; returns the value label. */
lv_obj_t* nav(lv_obj_t* list, const char* glyph, const char* title, const char* value, lv_event_cb_t on_click,
              void* user);

/** Read-only row; returns the value label. */
lv_obj_t* info(lv_obj_t* list, const char* glyph, const char* title, const char* value);

/** Row with a switch; returns the switch (LV_EVENT_VALUE_CHANGED). */
lv_obj_t* toggle(lv_obj_t* list, const char* glyph, const char* title, const char* subtitle, bool on,
                 lv_event_cb_t on_change, void* user);

struct Slider {
  lv_obj_t* row;
  lv_obj_t* title;
  lv_obj_t* slider;
  lv_obj_t* value;
};

/** Row with a slider below the title (LV_EVENT_VALUE_CHANGED on the slider). */
Slider slider(lv_obj_t* list, const char* glyph, const char* title, int32_t min, int32_t max, int32_t value,
              lv_event_cb_t on_change, void* user);

/**
 * Row with a segmented control. `options` is a button-matrix map (NULL
 * terminated, no "\n"). Returns the button matrix (LV_EVENT_VALUE_CHANGED,
 * use lv_buttonmatrix_get_selected_button()).
 */
lv_obj_t* segmented(lv_obj_t* list, const char* glyph, const char* title, const char* const* options,
                    uint32_t selected, lv_event_cb_t on_change, void* user);

/** Stepper: [-] value [+]. The callback receives the new value. */
using StepFormatFn = void (*)(int32_t value, char* out, uint32_t len);
using StepChangeFn = void (*)(int32_t value, void* user);

lv_obj_t* stepper(lv_obj_t* list, const char* glyph, const char* title, int32_t min, int32_t max, int32_t step,
                  int32_t value, StepFormatFn format, StepChangeFn on_change, void* user);

/** Clickable action row (e.g. "Factory reset"). */
lv_obj_t* action(lv_obj_t* list, const char* glyph, const char* title, lv_color_t color, lv_event_cb_t on_click,
                 void* user);

/** Explanatory text below a group. */
lv_obj_t* note(lv_obj_t* list, const char* text);

}  // namespace ui::rows
