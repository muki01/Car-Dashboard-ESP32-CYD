/**
 * @file dialog.h
 * Modal dialogs on lv_layer_top(): confirmation dialogs and content sheets.
 *
 * Tapping the dimmed backdrop cancels. Only one dialog is open at a time;
 * opening a new one replaces the previous.
 */
#pragma once

#include <lvgl.h>

namespace ui::dialog {

using ConfirmFn = void (*)(void* user);

/** Confirmation dialog. `on_confirm` runs only if the user confirms. */
void confirm(const char* title, const char* text, const char* confirm_text, bool destructive,
             ConfirmFn on_confirm, void* user);

/**
 * Opens a sheet with a title bar and a close button.
 * Returns the scrollable body (flex column) to be filled by the caller.
 */
lv_obj_t* sheet(const char* title);

void close();
bool is_open();

}  // namespace ui::dialog
