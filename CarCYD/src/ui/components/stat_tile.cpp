#include "stat_tile.h"

#include "../../core/i18n.h"
#include "../../core/signals.h"
#include "../signal_format.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui {

void StatTile::create(lv_obj_t* parent, bool align_right) {
  const lv_flex_align_t side = align_right ? LV_FLEX_ALIGN_END : LV_FLEX_ALIGN_START;

  root_ = column(parent, 0);
  lv_obj_set_size(root_, kWidth, kHeight);
  lv_obj_set_clickable(root_, true);
  lv_obj_set_flex_align(root_, LV_FLEX_ALIGN_START, side, side);

  // Line 1: value + unit (baseline aligned by padding the smaller font).
  lv_obj_t* line = row(root_, 2);
  lv_obj_set_flex_align(line, side, LV_FLEX_ALIGN_END, LV_FLEX_ALIGN_END);
  value_ = label(line, font::value(), color::text(), kNoValue);
  unit_ = label(line, font::caption(), color::text_dim(), "");
  lv_obj_set_style_pad_bottom(unit_, 3, 0);

  // Line 2: icon + caption.
  lv_obj_t* caption_line = row(root_, 3);
  lv_obj_set_flex_align(caption_line, side, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  icon_ = icon(caption_line, ICON_INFO, font::caption(), color::text_dim());
  caption_ = label(caption_line, font::caption(), color::text_faint(), "");
}

void StatTile::set_signal(core::SignalId id) {
  signal_ = id;
  set_text(icon_, signal_icon(id));
  set_text(caption_, core::tr(core::signal_info(id).short_name));
}

void StatTile::update(uint32_t now_ms) {
  const SignalText t = format_signal(signal_, now_ms);
  set_text(value_, t.value);
  set_text(unit_, t.valid ? t.unit : "");
  set_text_color(value_, t.valid ? color::severity(t.severity) : color::text_faint());
  set_text_color(icon_, t.severity >= core::Severity::Info ? color::severity(t.severity) : color::text_dim());
}

}  // namespace ui
