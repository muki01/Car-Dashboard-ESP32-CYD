#include "banner.h"

#include "../../core/platform.h"
#include "../../core/util.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui::banner {
namespace {

constexpr int32_t kHeight = 50;
constexpr int32_t kShownY = 4;
constexpr int32_t kHiddenY = -kHeight - 8;

lv_obj_t* g_root = nullptr;
lv_obj_t* g_stripe = nullptr;
lv_obj_t* g_icon = nullptr;
lv_obj_t* g_title = nullptr;
lv_obj_t* g_text = nullptr;
TapFn g_on_tap = nullptr;
void* g_user = nullptr;
uint32_t g_duration = 0;
uint32_t g_shown_at = 0;
bool g_visible = false;
int g_owner = -1;

void anim_y_cb(void* obj, int32_t v) { lv_obj_set_y(static_cast<lv_obj_t*>(obj), v); }

void hidden_cb(lv_anim_t*) {
  if (!g_visible) lv_obj_set_hidden(g_root, true);
}

void slide(int32_t to, bool hide_at_end) {
  lv_anim_t a;
  lv_anim_init(&a);
  lv_anim_set_var(&a, g_root);
  lv_anim_set_exec_cb(&a, anim_y_cb);
  lv_anim_set_values(&a, lv_obj_get_y(g_root), to);
  lv_anim_set_duration(&a, motion::kNormal);
  lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
  if (hide_at_end) lv_anim_set_completed_cb(&a, hidden_cb);
  lv_anim_start(&a);
}

void clicked_cb(lv_event_t*) {
  const TapFn fn = g_on_tap;
  void* user = g_user;
  hide();
  if (fn != nullptr) fn(user);
}

}  // namespace

void init() {
  g_root = lv_obj_create(lv_layer_top());
  lv_obj_remove_style_all(g_root);
  lv_obj_set_size(g_root, layout::kContentW - 2 * layout::kPad, kHeight);
  lv_obj_set_pos(g_root, layout::kRailW + layout::kPad, kHiddenY);
  lv_obj_set_style_bg_color(g_root, color::card_hi(), 0);
  lv_obj_set_style_bg_opa(g_root, LV_OPA_COVER, 0);
  lv_obj_set_style_radius(g_root, layout::kRadius, 0);
  lv_obj_set_style_border_width(g_root, 1, 0);
  lv_obj_set_style_pad_left(g_root, 14, 0);
  lv_obj_set_style_pad_right(g_root, 10, 0);
  lv_obj_set_style_clip_corner(g_root, true, 0);
  lv_obj_set_scrollable(g_root, false);
  lv_obj_set_clickable(g_root, true);
  lv_obj_set_hidden(g_root, true);
  lv_obj_add_event_cb(g_root, clicked_cb, LV_EVENT_CLICKED, nullptr);
  add_touch_feedback(g_root);

  g_stripe = box(g_root);
  lv_obj_set_ignore_layout(g_stripe, true);
  lv_obj_set_size(g_stripe, 4, kHeight);
  lv_obj_set_pos(g_stripe, -14, -1);
  lv_obj_set_style_bg_opa(g_stripe, LV_OPA_COVER, 0);

  lv_obj_set_flex_flow(g_root, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(g_root, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  lv_obj_set_style_pad_column(g_root, 10, 0);

  g_icon = icon(g_root, ICON_INFO, font::value(), color::info());
  lv_obj_t* texts = column(g_root, 0);
  lv_obj_set_flex_grow(texts, 1);
  g_title = label(texts, font::title(), color::text(), "");
  single_line(g_title);
  lv_obj_set_width(g_title, lv_pct(100));
  g_text = label(texts, font::caption(), color::text_dim(), "");
  single_line(g_text);
  lv_obj_set_width(g_text, lv_pct(100));
}

void deinit() {
  if (g_root == nullptr) return;
  lv_anim_delete(g_root, nullptr);
  lv_obj_delete(g_root);
  g_root = g_stripe = g_icon = g_title = g_text = nullptr;
  g_visible = false;
  g_owner = -1;
  g_on_tap = nullptr;
}

void show(core::Severity severity, const char* glyph, const char* title, const char* text, uint32_t duration_ms,
          TapFn on_tap, void* user) {
  if (g_root == nullptr) return;
  const lv_color_t c = severity == core::Severity::Normal ? color::ok() : color::severity(severity);
  lv_obj_set_style_bg_color(g_stripe, c, 0);
  lv_obj_set_style_border_color(g_root, lv_color_mix(c, color::stroke(), LV_OPA_40), 0);
  lv_obj_set_style_bg_color(g_root, severity == core::Severity::Critical ? lv_color_hex(0x3A1216) : color::card_hi(), 0);
  lv_label_set_text(g_icon, glyph);
  lv_obj_set_style_text_color(g_icon, c, 0);
  lv_label_set_text(g_title, title);
  lv_label_set_text(g_text, text != nullptr ? text : "");
  set_visible(g_text, text != nullptr && text[0] != '\0');

  g_on_tap = on_tap;
  g_user = user;
  g_duration = duration_ms;
  g_shown_at = platform::millis();
  g_owner = -1;

  lv_obj_move_foreground(g_root);
  lv_obj_set_hidden(g_root, false);
  if (!g_visible) {
    g_visible = true;
    slide(kShownY, false);
  }
}

void hide() {
  if (!g_visible) return;
  g_visible = false;
  g_owner = -1;
  slide(kHiddenY, true);
}

bool visible() { return g_visible; }

int owner() { return g_owner; }
void set_owner(int owner) { g_owner = owner; }

void tick(uint32_t now_ms) {
  if (g_visible && g_duration > 0 && core::elapsed(now_ms, g_shown_at) >= g_duration) hide();
}

}  // namespace ui::banner
