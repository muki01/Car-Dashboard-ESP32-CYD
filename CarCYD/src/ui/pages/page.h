/**
 * @file page.h
 * Base class of the pages shown in the content area.
 */
#pragma once

#include <lvgl.h>
#include <stdint.h>

namespace ui {

class Page {
 public:
  virtual ~Page() = default;

  /** Builds the widgets inside `parent` (the content area) and sets root_. */
  virtual void create(lv_obj_t* parent) = 0;

  /** Title shown in the status bar. */
  virtual const char* title() const = 0;

  virtual void on_show() {}
  virtual void on_hide() {}

  /** Periodic refresh while visible (called every UI tick, ~25 Hz). */
  virtual void update(uint32_t now_ms) = 0;

  /** Status bar back button. Return true if a sub-view was closed. */
  virtual bool on_back() { return false; }

  /** Settings were committed (core::settings::Change mask). */
  virtual void on_settings_changed(uint32_t changes) { (void)changes; }

  /** Opaque navigation state preserved across UI rebuilds (-1 = none). */
  virtual int save_state() const { return -1; }
  virtual void restore_state(int state) { (void)state; }

  lv_obj_t* root() const { return root_; }

 protected:
  lv_obj_t* root_ = nullptr;
};

}  // namespace ui
