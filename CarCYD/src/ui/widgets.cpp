#include "widgets.h"

#include <string.h>

#include "../core/platform.h"

namespace ui {
namespace {

void feedback_cb(lv_event_t*) { platform::play(platform::Sound::Click); }

}  // namespace

lv_obj_t* box(lv_obj_t* parent) {
  lv_obj_t* obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_set_scrollable(obj, false);
  lv_obj_set_clickable(obj, false);
  lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
  return obj;
}

lv_obj_t* row(lv_obj_t* parent, int32_t gap) {
  lv_obj_t* obj = box(parent);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(obj, gap, 0);
  return obj;
}

lv_obj_t* column(lv_obj_t* parent, int32_t gap) {
  lv_obj_t* obj = box(parent);
  lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(obj, gap, 0);
  return obj;
}

lv_obj_t* label(lv_obj_t* parent, const lv_font_t* font, lv_color_t color, const char* text) {
  lv_obj_t* obj = lv_label_create(parent);
  lv_obj_set_style_text_font(obj, font, 0);
  lv_obj_set_style_text_color(obj, color, 0);
  lv_label_set_text(obj, text);
  return obj;
}

void single_line(lv_obj_t* label) {
  lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_DOTS);
  lv_obj_set_height(label, lv_font_get_line_height(lv_obj_get_style_text_font(label, LV_PART_MAIN)));
}

lv_obj_t* icon(lv_obj_t* parent, const char* glyph, const lv_font_t* font, lv_color_t color) {
  return label(parent, font, color, glyph);
}

lv_obj_t* card(lv_obj_t* parent) {
  lv_obj_t* obj = lv_obj_create(parent);
  lv_obj_remove_style_all(obj);
  lv_obj_set_scrollable(obj, false);
  lv_obj_set_clickable(obj, false);
  lv_obj_set_style_bg_color(obj, color::card(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(obj, layout::kRadius, 0);
  lv_obj_set_style_pad_all(obj, layout::kPad, 0);
  return obj;
}

lv_obj_t* button(lv_obj_t* parent, const char* glyph, const char* text, ButtonStyle style, lv_event_cb_t on_click,
                 void* user) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_remove_style_all(btn);
  lv_obj_set_height(btn, layout::kButtonH);
  lv_obj_set_style_radius(btn, layout::kRadiusSm + 2, 0);
  lv_obj_set_style_pad_hor(btn, 14, 0);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
  lv_obj_set_flex_flow(btn, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(btn, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(btn, 6, 0);

  lv_color_t bg = color::card_hi();
  lv_color_t fg = color::text();
  switch (style) {
    case ButtonStyle::Primary:
      bg = color::accent();
      fg = color::on_accent();
      break;
    case ButtonStyle::Danger:
      bg = color::crit();
      fg = lv_color_white();
      break;
    case ButtonStyle::Ghost:
      lv_obj_set_style_bg_opa(btn, LV_OPA_TRANSP, 0);
      lv_obj_set_style_border_width(btn, 1, 0);
      lv_obj_set_style_border_color(btn, color::stroke(), 0);
      fg = color::text();
      break;
    case ButtonStyle::Secondary:
      break;
  }
  lv_obj_set_style_bg_color(btn, bg, 0);
  lv_obj_set_style_text_color(btn, fg, 0);
  lv_obj_set_style_text_font(btn, font::title(), 0);

  // Pressed: slightly darker; disabled: faded.
  lv_obj_set_style_bg_color(btn, lv_color_mix(bg, lv_color_black(), LV_OPA_70), LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);
  lv_obj_set_style_opa(btn, LV_OPA_40, LV_STATE_DISABLED);

  if (glyph != nullptr) {
    lv_obj_t* ic = lv_label_create(btn);
    lv_label_set_text(ic, glyph);
    lv_obj_set_style_text_font(ic, font::value(), 0);
  }
  if (text != nullptr) {
    lv_obj_t* lbl = lv_label_create(btn);
    lv_label_set_text(lbl, text);
  }
  if (on_click != nullptr) lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, user);
  add_touch_feedback(btn);
  return btn;
}

lv_obj_t* icon_button(lv_obj_t* parent, const char* glyph, int32_t size, lv_event_cb_t on_click, void* user) {
  lv_obj_t* btn = lv_button_create(parent);
  lv_obj_remove_style_all(btn);
  lv_obj_set_size(btn, size, size);
  lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_bg_color(btn, color::card_hi(), LV_STATE_PRESSED);
  lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_PRESSED);
  lv_obj_set_style_text_color(btn, color::text(), 0);
  lv_obj_set_style_text_font(btn, font::value(), 0);
  lv_obj_set_style_opa(btn, LV_OPA_40, LV_STATE_DISABLED);

  const int32_t extra = size < layout::kTouchMin ? (layout::kTouchMin - size) / 2 + 2 : 2;
  lv_obj_set_ext_click_area(btn, extra);

  lv_obj_t* ic = lv_label_create(btn);
  lv_label_set_text(ic, glyph);
  lv_obj_center(ic);

  if (on_click != nullptr) lv_obj_add_event_cb(btn, on_click, LV_EVENT_CLICKED, user);
  add_touch_feedback(btn);
  return btn;
}

lv_obj_t* chip(lv_obj_t* parent, const char* text, lv_color_t color) {
  lv_obj_t* obj = box(parent);
  lv_obj_set_style_radius(obj, layout::kRadiusSm, 0);
  lv_obj_set_style_pad_hor(obj, 6, 0);
  lv_obj_set_style_pad_ver(obj, 1, 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_20, 0);
  label(obj, font::caption(), color, text);
  chip_set(obj, text, color);
  return obj;
}

void chip_set(lv_obj_t* chip, const char* text, lv_color_t color) {
  lv_obj_set_style_bg_color(chip, color, 0);
  lv_obj_t* lbl = lv_obj_get_child(chip, 0);
  set_text(lbl, text);
  set_text_color(lbl, color);
}

lv_obj_t* divider(lv_obj_t* parent) {
  lv_obj_t* obj = box(parent);
  lv_obj_set_size(obj, lv_pct(100), 1);
  lv_obj_set_style_bg_color(obj, color::stroke(), 0);
  lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
  return obj;
}

void add_touch_feedback(lv_obj_t* obj) { lv_obj_add_event_cb(obj, feedback_cb, LV_EVENT_PRESSED, nullptr); }

void set_text(lv_obj_t* label, const char* text) {
  const char* current = lv_label_get_text(label);
  if (current == nullptr || strcmp(current, text) != 0) lv_label_set_text(label, text);
}

void set_text_color(lv_obj_t* obj, lv_color_t color) {
  if (!lv_color_eq(lv_obj_get_style_text_color(obj, LV_PART_MAIN), color)) {
    lv_obj_set_style_text_color(obj, color, 0);
  }
}

void set_visible(lv_obj_t* obj, bool visible) {
  if (visible == lv_obj_is_hidden(obj)) lv_obj_set_hidden(obj, !visible);
}

}  // namespace ui
