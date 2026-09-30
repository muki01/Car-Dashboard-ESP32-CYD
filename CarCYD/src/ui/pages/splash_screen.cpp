#include "splash_screen.h"

#include "../../config/app_config.h"
#include "../../core/i18n.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui::splash {
namespace {

lv_obj_t* g_arc = nullptr;
lv_obj_t* g_status = nullptr;

}  // namespace

lv_obj_t* create() {
  lv_obj_t* scr = lv_obj_create(nullptr);
  lv_obj_remove_style_all(scr);
  lv_obj_set_style_bg_color(scr, color::bg(), 0);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
  lv_obj_set_scrollable(scr, false);

  // Logo: a gauge arc that fills with the boot progress.
  g_arc = lv_arc_create(scr);
  lv_obj_remove_style_all(g_arc);
  lv_obj_set_size(g_arc, 96, 96);
  lv_obj_align(g_arc, LV_ALIGN_CENTER, 0, -36);
  lv_obj_set_clickable(g_arc, false);
  lv_arc_set_rotation(g_arc, 135);
  lv_arc_set_bg_angles(g_arc, 0, 270);
  lv_arc_set_range(g_arc, 0, 100);
  lv_arc_set_value(g_arc, 0);
  lv_obj_set_style_arc_width(g_arc, 7, LV_PART_MAIN);
  lv_obj_set_style_arc_color(g_arc, color::track(), LV_PART_MAIN);
  lv_obj_set_style_arc_width(g_arc, 7, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(g_arc, color::accent(), LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(g_arc, true, LV_PART_INDICATOR);
  lv_obj_set_style_arc_rounded(g_arc, true, LV_PART_MAIN);

  lv_obj_t* logo = icon(g_arc, ICON_DASHBOARD, font::icon48(), color::text());
  lv_obj_center(logo);

  lv_obj_t* name = label(scr, font::large(), color::text(), APP_NAME);
  lv_obj_align(name, LV_ALIGN_CENTER, 0, 34);
  lv_obj_t* tagline = label(scr, font::body(), color::text_dim(), core::tr(core::Str::APP_TAGLINE));
  lv_obj_align_to(tagline, name, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

  g_status = label(scr, font::caption(), color::text_faint(), "");
  lv_obj_align(g_status, LV_ALIGN_BOTTOM_LEFT, 12, -10);
  lv_obj_t* version = label(scr, font::caption(), color::text_faint(), "v" APP_VERSION);
  lv_obj_align(version, LV_ALIGN_BOTTOM_RIGHT, -12, -10);

  lv_screen_load(scr);
  return scr;
}

void set_progress(uint8_t percent, const char* text) {
  if (g_arc == nullptr) return;
  lv_arc_set_value(g_arc, percent);
  lv_label_set_text(g_status, text != nullptr ? text : "");
}

}  // namespace ui::splash
