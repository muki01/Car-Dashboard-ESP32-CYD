#include "dialog.h"

#include "../../core/i18n.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui::dialog {
namespace {

struct ConfirmData {
  ConfirmFn fn;
  void* user;
};

lv_obj_t* g_backdrop = nullptr;

void backdrop_deleted_cb(lv_event_t* e) {
  if (lv_event_get_target_obj(e) == g_backdrop) g_backdrop = nullptr;
  delete static_cast<ConfirmData*>(lv_event_get_user_data(e));
}

void backdrop_clicked_cb(lv_event_t* e) {
  // Only taps on the backdrop itself (not bubbled from the card) cancel.
  if (lv_event_get_target_obj(e) == lv_event_get_current_target_obj(e)) close();
}

void close_cb(lv_event_t*) { close(); }

void confirm_cb(lv_event_t* e) {
  auto* data = static_cast<ConfirmData*>(lv_event_get_user_data(e));
  const ConfirmFn fn = data->fn;
  void* user = data->user;
  close();
  if (fn != nullptr) fn(user);
}

lv_obj_t* open_backdrop(ConfirmData* data) {
  close();
  g_backdrop = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(g_backdrop);
  lv_obj_set_size(g_backdrop, layout::kScreenW, layout::kScreenH);
  lv_obj_set_style_bg_color(g_backdrop, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(g_backdrop, LV_OPA_60, 0);
  lv_obj_set_clickable(g_backdrop, true);
  lv_obj_set_scrollable(g_backdrop, false);
  lv_obj_add_event_cb(g_backdrop, backdrop_clicked_cb, LV_EVENT_CLICKED, nullptr);
  lv_obj_add_event_cb(g_backdrop, backdrop_deleted_cb, LV_EVENT_DELETE, data);
  return g_backdrop;
}

lv_obj_t* make_card(lv_obj_t* parent, int32_t width) {
  lv_obj_t* c = card(parent);
  lv_obj_set_width(c, width);
  lv_obj_set_height(c, LV_SIZE_CONTENT);
  lv_obj_set_style_bg_color(c, color::card(), 0);
  lv_obj_set_style_border_width(c, 1, 0);
  lv_obj_set_style_border_color(c, color::stroke(), 0);
  lv_obj_set_style_radius(c, 12, 0);
  lv_obj_set_style_pad_all(c, 14, 0);
  lv_obj_set_clickable(c, true);  // swallow clicks so they do not reach the backdrop
  lv_obj_center(c);
  return c;
}

}  // namespace

void confirm(const char* title, const char* text, const char* confirm_text, bool destructive, ConfirmFn on_confirm,
             void* user) {
  auto* data = new ConfirmData{on_confirm, user};
  lv_obj_t* backdrop = open_backdrop(data);
  lv_obj_t* c = make_card(backdrop, 272);
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 8, 0);

  label(c, font::title(), color::text(), title);
  lv_obj_t* body = label(c, font::body(), color::text_dim(), text);
  lv_obj_set_width(body, lv_pct(100));
  lv_label_set_long_mode(body, LV_LABEL_LONG_MODE_WRAP);

  lv_obj_t* actions = row(c, 8);
  lv_obj_set_width(actions, lv_pct(100));
  lv_obj_set_style_pad_top(actions, 6, 0);

  lv_obj_t* cancel = button(actions, nullptr, core::tr(core::Str::CANCEL), ButtonStyle::Secondary, close_cb, nullptr);
  lv_obj_set_flex_grow(cancel, 1);
  lv_obj_t* ok = button(actions, nullptr, confirm_text, destructive ? ButtonStyle::Danger : ButtonStyle::Primary,
                        confirm_cb, data);
  lv_obj_set_flex_grow(ok, 1);
}

lv_obj_t* sheet(const char* title) {
  lv_obj_t* backdrop = open_backdrop(nullptr);
  lv_obj_t* c = make_card(backdrop, 284);
  lv_obj_set_style_pad_all(c, 10, 0);
  lv_obj_set_style_max_height(c, layout::kScreenH - 16, 0);
  lv_obj_set_flex_flow(c, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(c, 6, 0);

  lv_obj_t* header = row(c, 6);
  lv_obj_set_width(header, lv_pct(100));
  lv_obj_t* t = label(header, font::title(), color::text(), title);
  lv_obj_set_flex_grow(t, 1);
  lv_obj_set_style_pad_left(t, 4, 0);
  icon_button(header, ICON_CLOSE, 30, close_cb, nullptr);

  // Content-sized body that starts scrolling once the sheet reaches the screen height.
  lv_obj_t* body = column(c, 4);
  lv_obj_set_width(body, lv_pct(100));
  lv_obj_set_scrollable(body, true);
  lv_obj_set_scroll_dir(body, LV_DIR_VER);
  lv_obj_set_scrollbar_mode(body, LV_SCROLLBAR_MODE_ACTIVE);
  lv_obj_set_style_max_height(body, layout::kScreenH - 70, 0);
  return body;
}

void close() {
  if (g_backdrop != nullptr) {
    lv_obj_t* b = g_backdrop;
    g_backdrop = nullptr;
    lv_obj_delete_async(b);
  }
}

bool is_open() { return g_backdrop != nullptr; }

}  // namespace ui::dialog
