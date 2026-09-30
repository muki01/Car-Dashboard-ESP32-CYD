/**
 * @file nav_rail.h
 * Vertical navigation rail on the left (driver) side of the screen.
 */
#pragma once

#include <lvgl.h>

#include "../ui.h"

namespace ui {

class NavRail {
 public:
  using SelectFn = void (*)(PageId page);

  void create(lv_obj_t* parent, SelectFn on_select);
  void set_active(PageId page);

  /** Small dot on an item (e.g. check engine light on the Diagnostics item). */
  void set_badge(PageId page, bool visible, lv_color_t color);

 private:
  static void clicked_cb(lv_event_t* e);

  static constexpr int kItems = static_cast<int>(PageId::Count);
  lv_obj_t* items_[kItems] = {};
  lv_obj_t* icons_[kItems] = {};
  lv_obj_t* markers_[kItems] = {};
  lv_obj_t* badges_[kItems] = {};
  SelectFn on_select_ = nullptr;
};

}  // namespace ui
