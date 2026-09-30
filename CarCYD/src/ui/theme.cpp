#include "theme.h"

#include "../core/settings.h"

namespace ui {
namespace color {

lv_color_t bg() { return lv_color_hex(0x06080C); }
lv_color_t surface() { return lv_color_hex(0x0C1016); }
lv_color_t card() { return lv_color_hex(0x131922); }
lv_color_t card_hi() { return lv_color_hex(0x1C2430); }
lv_color_t stroke() { return lv_color_hex(0x232C39); }
lv_color_t track() { return lv_color_hex(0x1A212B); }
lv_color_t text() { return lv_color_hex(0xE9EEF4); }
lv_color_t text_dim() { return lv_color_hex(0x8B97A7); }
lv_color_t text_faint() { return lv_color_hex(0x4F5B6C); }
lv_color_t ok() { return lv_color_hex(0x2ED47A); }
lv_color_t info() { return lv_color_hex(0x3DA5FF); }
lv_color_t warn() { return lv_color_hex(0xFFB020); }
lv_color_t crit() { return lv_color_hex(0xFF4B4B); }
lv_color_t on_accent() { return lv_color_hex(0x071017); }

lv_color_t accent_of(core::Accent accent) {
  switch (accent) {
    case core::Accent::Amber: return lv_color_hex(0xFF8A1F);
    case core::Accent::Red: return lv_color_hex(0xF23A4B);
    case core::Accent::Green: return lv_color_hex(0x25D38A);
    case core::Accent::Violet: return lv_color_hex(0x9D84FF);
    case core::Accent::Cyan:
    default: return lv_color_hex(0x1FCBF2);
  }
}

lv_color_t accent() { return accent_of(core::settings::get().accent); }

lv_color_t severity(core::Severity s) {
  switch (s) {
    case core::Severity::Info: return info();
    case core::Severity::Warning: return warn();
    case core::Severity::Critical: return crit();
    case core::Severity::Normal:
    default: return text();
  }
}

lv_color_t severity_accent(core::Severity s) { return s == core::Severity::Normal ? accent() : severity(s); }

}  // namespace color

void theme_apply(lv_display_t* display) {
  lv_theme_t* theme = lv_theme_default_init(display, color::accent(), color::info(), true, font::body());
  lv_display_set_theme(display, theme);
}

}  // namespace ui
