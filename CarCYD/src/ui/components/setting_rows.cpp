#include "setting_rows.h"

#include "../../core/util.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui::rows {
namespace {

lv_obj_t* base_row(lv_obj_t* list, bool clickable) {
  lv_obj_t* r = card(list);
  lv_obj_set_width(r, lv_pct(100));
  lv_obj_set_height(r, LV_SIZE_CONTENT);
  lv_obj_set_style_min_height(r, layout::kRowH, 0);
  lv_obj_set_style_pad_ver(r, 6, 0);
  lv_obj_set_style_pad_hor(r, 10, 0);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(r, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(r, 10, 0);
  if (clickable) {
    lv_obj_set_clickable(r, true);
    lv_obj_set_style_bg_color(r, color::card_hi(), LV_STATE_PRESSED);
    add_touch_feedback(r);
  }
  return r;
}

lv_obj_t* row_icon(lv_obj_t* r, const char* glyph) {
  lv_obj_t* ic = icon(r, glyph, font::value(), color::accent());
  lv_obj_set_width(ic, 22);
  return ic;
}

lv_obj_t* row_title(lv_obj_t* parent, const char* title) {
  lv_obj_t* t = label(parent, font::body(), color::text(), title);
  lv_obj_set_flex_grow(t, 1);
  single_line(t);
  return t;
}

/** Row with icon + title on the first line and a full-width control below. */
lv_obj_t* stacked_row(lv_obj_t* list, const char* glyph, const char* title, lv_obj_t** header_out,
                      lv_obj_t** title_out = nullptr) {
  lv_obj_t* r = base_row(list, false);
  lv_obj_set_flex_flow(r, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(r, 8, 0);
  lv_obj_set_style_pad_ver(r, 8, 0);
  lv_obj_t* header = row(r, 10);
  lv_obj_set_width(header, lv_pct(100));
  row_icon(header, glyph);
  lv_obj_t* t = row_title(header, title);
  if (header_out != nullptr) *header_out = header;
  if (title_out != nullptr) *title_out = t;
  return r;
}

// ---- Stepper -------------------------------------------------------------------------

struct StepperState {
  int32_t min, max, step, value;
  StepFormatFn format;
  StepChangeFn on_change;
  void* user;
  lv_obj_t* label;
  lv_obj_t* minus;
  lv_obj_t* plus;
};

void stepper_refresh(StepperState* s) {
  char buf[24];
  s->format(s->value, buf, sizeof(buf));
  set_text(s->label, buf);
  if (s->value <= s->min) {
    lv_obj_add_state(s->minus, LV_STATE_DISABLED);
  } else {
    lv_obj_remove_state(s->minus, LV_STATE_DISABLED);
  }
  if (s->value >= s->max) {
    lv_obj_add_state(s->plus, LV_STATE_DISABLED);
  } else {
    lv_obj_remove_state(s->plus, LV_STATE_DISABLED);
  }
}

void stepper_step(lv_event_t* e, int32_t direction) {
  auto* s = static_cast<StepperState*>(lv_event_get_user_data(e));
  const int32_t next = core::clamp(s->value + direction * s->step, s->min, s->max);
  if (next == s->value) return;
  s->value = next;
  stepper_refresh(s);
  if (s->on_change != nullptr) s->on_change(s->value, s->user);
}

void stepper_minus_cb(lv_event_t* e) { stepper_step(e, -1); }
void stepper_plus_cb(lv_event_t* e) { stepper_step(e, +1); }
void stepper_delete_cb(lv_event_t* e) { delete static_cast<StepperState*>(lv_event_get_user_data(e)); }

}  // namespace

lv_obj_t* list(lv_obj_t* parent) {
  lv_obj_t* l = column(parent, layout::kGap);
  lv_obj_set_size(l, lv_pct(100), lv_pct(100));
  lv_obj_set_style_pad_all(l, layout::kPad, 0);
  lv_obj_set_style_pad_bottom(l, layout::kPad + 4, 0);
  lv_obj_set_scrollable(l, true);
  lv_obj_set_scroll_dir(l, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(l, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_bg_color(l, color::text_faint(), LV_PART_SCROLLBAR);
  lv_obj_set_style_bg_opa(l, LV_OPA_COVER, LV_PART_SCROLLBAR);
  lv_obj_set_style_width(l, 3, LV_PART_SCROLLBAR);
  lv_obj_set_style_pad_right(l, 2, LV_PART_SCROLLBAR);
  return l;
}

lv_obj_t* section(lv_obj_t* list, const char* text) {
  lv_obj_t* s = label(list, font::caption(), color::text_dim(), text);
  lv_obj_set_style_pad_top(s, 4, 0);
  lv_obj_set_style_pad_left(s, 4, 0);
  return s;
}

lv_obj_t* nav(lv_obj_t* list, const char* glyph, const char* title, const char* value, lv_event_cb_t on_click,
              void* user) {
  lv_obj_t* r = base_row(list, true);
  row_icon(r, glyph);
  row_title(r, title);
  lv_obj_t* v = label(r, font::caption(), color::text_dim(), value != nullptr ? value : "");
  icon(r, ICON_CHEVRON_RIGHT, font::title(), color::text_faint());
  if (on_click != nullptr) lv_obj_add_event_cb(r, on_click, LV_EVENT_CLICKED, user);
  return v;
}

lv_obj_t* info(lv_obj_t* list, const char* glyph, const char* title, const char* value) {
  lv_obj_t* r = base_row(list, false);
  lv_obj_set_style_min_height(r, 38, 0);
  row_icon(r, glyph);
  row_title(r, title);
  // Long values are truncated so the title always stays readable.
  lv_obj_t* v = label(r, font::body(), color::text_dim(), value != nullptr ? value : "");
  lv_obj_set_style_max_width(v, lv_pct(62), 0);
  single_line(v);
  return v;
}

lv_obj_t* toggle(lv_obj_t* list, const char* glyph, const char* title, const char* subtitle, bool on,
                 lv_event_cb_t on_change, void* user) {
  lv_obj_t* r = base_row(list, false);
  row_icon(r, glyph);
  lv_obj_t* texts = column(r, 0);
  lv_obj_set_flex_grow(texts, 1);
  lv_obj_t* t = label(texts, font::body(), color::text(), title);
  lv_obj_set_width(t, lv_pct(100));
  single_line(t);
  if (subtitle != nullptr) {
    lv_obj_t* s = label(texts, font::caption(), color::text_dim(), subtitle);
    lv_obj_set_width(s, lv_pct(100));
    single_line(s);
  }

  lv_obj_t* sw = lv_switch_create(r);
  lv_obj_set_size(sw, 46, 26);
  lv_obj_set_ext_click_area(sw, 8);
  lv_obj_set_style_bg_color(sw, color::card_hi(), LV_PART_MAIN);
  lv_obj_set_style_bg_color(sw, color::accent(), selector(LV_PART_INDICATOR, LV_STATE_CHECKED));
  if (on) lv_obj_add_state(sw, LV_STATE_CHECKED);
  if (on_change != nullptr) lv_obj_add_event_cb(sw, on_change, LV_EVENT_VALUE_CHANGED, user);
  add_touch_feedback(sw);
  return sw;
}

Slider slider(lv_obj_t* list, const char* glyph, const char* title, int32_t min, int32_t max, int32_t value,
              lv_event_cb_t on_change, void* user) {
  lv_obj_t* header = nullptr;
  Slider out{};
  out.row = stacked_row(list, glyph, title, &header, &out.title);
  out.value = label(header, font::body(), color::text_dim(), "");

  out.slider = lv_slider_create(out.row);
  lv_obj_set_width(out.slider, lv_pct(92));
  lv_obj_set_height(out.slider, 6);
  lv_obj_set_style_margin_bottom(out.slider, 6, 0);
  lv_obj_set_ext_click_area(out.slider, 14);
  lv_obj_set_style_bg_color(out.slider, color::card_hi(), LV_PART_MAIN);
  lv_obj_set_style_bg_color(out.slider, color::accent(), LV_PART_INDICATOR);
  lv_obj_set_style_bg_color(out.slider, color::text(), LV_PART_KNOB);
  lv_obj_set_style_pad_all(out.slider, 6, LV_PART_KNOB);
  lv_slider_set_range(out.slider, min, max);
  lv_slider_set_value(out.slider, value, LV_ANIM_OFF);
  if (on_change != nullptr) lv_obj_add_event_cb(out.slider, on_change, LV_EVENT_VALUE_CHANGED, user);
  return out;
}

lv_obj_t* segmented(lv_obj_t* list, const char* glyph, const char* title, const char* const* options,
                    uint32_t selected, lv_event_cb_t on_change, void* user) {
  lv_obj_t* r = stacked_row(list, glyph, title, nullptr);

  lv_obj_t* bm = lv_buttonmatrix_create(r);
  lv_buttonmatrix_set_map(bm, options);
  lv_buttonmatrix_set_button_ctrl_all(bm, LV_BUTTONMATRIX_CTRL_CHECKABLE);
  lv_buttonmatrix_set_one_checked(bm, true);
  lv_buttonmatrix_set_button_ctrl(bm, selected, LV_BUTTONMATRIX_CTRL_CHECKED);
  lv_buttonmatrix_set_selected_button(bm, selected);
  lv_obj_set_size(bm, lv_pct(100), 34);

  lv_obj_set_style_bg_color(bm, color::bg(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(bm, LV_OPA_COVER, LV_PART_MAIN);
  lv_obj_set_style_border_width(bm, 0, LV_PART_MAIN);
  lv_obj_set_style_radius(bm, layout::kRadiusSm + 2, LV_PART_MAIN);
  lv_obj_set_style_pad_all(bm, 3, LV_PART_MAIN);
  lv_obj_set_style_pad_gap(bm, 3, LV_PART_MAIN);

  lv_obj_set_style_bg_opa(bm, LV_OPA_TRANSP, LV_PART_ITEMS);
  lv_obj_set_style_border_width(bm, 0, LV_PART_ITEMS);
  lv_obj_set_style_shadow_width(bm, 0, LV_PART_ITEMS);
  lv_obj_set_style_radius(bm, layout::kRadiusSm, LV_PART_ITEMS);
  lv_obj_set_style_text_color(bm, color::text_dim(), LV_PART_ITEMS);
  lv_obj_set_style_text_font(bm, font::body(), LV_PART_ITEMS);
  lv_obj_set_style_bg_color(bm, color::accent(), selector(LV_PART_ITEMS, LV_STATE_CHECKED));
  lv_obj_set_style_bg_opa(bm, LV_OPA_COVER, selector(LV_PART_ITEMS, LV_STATE_CHECKED));
  lv_obj_set_style_text_color(bm, color::on_accent(), selector(LV_PART_ITEMS, LV_STATE_CHECKED));
  lv_obj_set_style_bg_color(bm, color::card_hi(), selector(LV_PART_ITEMS, LV_STATE_PRESSED));
  lv_obj_set_style_bg_opa(bm, LV_OPA_COVER, selector(LV_PART_ITEMS, LV_STATE_PRESSED));

  if (on_change != nullptr) lv_obj_add_event_cb(bm, on_change, LV_EVENT_VALUE_CHANGED, user);
  add_touch_feedback(bm);
  return bm;
}

lv_obj_t* stepper(lv_obj_t* list, const char* glyph, const char* title, int32_t min, int32_t max, int32_t step,
                  int32_t value, StepFormatFn format, StepChangeFn on_change, void* user) {
  // [icon] Title
  // [  -  ]      6500 rpm      [  +  ]
  lv_obj_t* r = stacked_row(list, glyph, title, nullptr);

  auto* s = new StepperState{min, max, step, core::clamp(value, min, max), format, on_change, user,
                             nullptr, nullptr, nullptr};
  lv_obj_add_event_cb(r, stepper_delete_cb, LV_EVENT_DELETE, s);

  lv_obj_t* controls = row(r, 8);
  lv_obj_set_width(controls, lv_pct(100));

  auto make_step_button = [&](const char* glyph_text, lv_event_cb_t cb) {
    lv_obj_t* b = icon_button(controls, glyph_text, 34, cb, s);
    lv_obj_set_width(b, 58);
    lv_obj_set_style_radius(b, layout::kRadiusSm + 2, 0);
    lv_obj_set_style_bg_color(b, color::card_hi(), 0);
    lv_obj_set_style_bg_opa(b, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(b, color::stroke(), LV_STATE_PRESSED);
    return b;
  };
  s->minus = make_step_button(ICON_MINUS, stepper_minus_cb);
  s->label = label(controls, font::title(), color::text(), "");
  lv_obj_set_flex_grow(s->label, 1);
  lv_obj_set_style_text_align(s->label, LV_TEXT_ALIGN_CENTER, 0);
  s->plus = make_step_button(ICON_PLUS, stepper_plus_cb);

  stepper_refresh(s);
  return r;
}

lv_obj_t* action(lv_obj_t* list, const char* glyph, const char* title, lv_color_t c, lv_event_cb_t on_click,
                 void* user) {
  lv_obj_t* r = base_row(list, true);
  lv_obj_t* ic = row_icon(r, glyph);
  lv_obj_set_style_text_color(ic, c, 0);
  lv_obj_t* t = row_title(r, title);
  lv_obj_set_style_text_color(t, c, 0);
  icon(r, ICON_CHEVRON_RIGHT, font::title(), color::text_faint());
  if (on_click != nullptr) lv_obj_add_event_cb(r, on_click, LV_EVENT_CLICKED, user);
  return r;
}

lv_obj_t* note(lv_obj_t* list, const char* text) {
  lv_obj_t* n = label(list, font::caption(), color::text_dim(), text);
  lv_obj_set_width(n, lv_pct(100));
  lv_label_set_long_mode(n, LV_LABEL_LONG_MODE_WRAP);
  lv_obj_set_style_pad_hor(n, 4, 0);
  return n;
}

}  // namespace ui::rows
