/**
 * @file diag_page.h
 * Diagnostics: check engine light status, reading and clearing of the
 * diagnostic trouble codes, with a detail sheet for every code.
 */
#pragma once

#include "page.h"

namespace ui {

class DiagPage final : public Page {
 public:
  void create(lv_obj_t* parent) override;
  const char* title() const override;
  void on_show() override;
  void update(uint32_t now_ms) override;

 private:
  void rebuild_content(uint32_t now_ms);
  void update_summary(uint32_t now_ms);
  void show_state(const char* glyph, lv_color_t color, const char* title, const char* text);
  void show_busy(const char* text);
  void show_codes();
  void report_result();

  static void scan_cb(lv_event_t* e);
  static void clear_cb(lv_event_t* e);
  static void clear_confirmed(void* user);
  static void code_clicked_cb(lv_event_t* e);

  lv_obj_t* mil_icon_ = nullptr;
  lv_obj_t* mil_state_ = nullptr;
  lv_obj_t* scan_info_ = nullptr;
  lv_obj_t* scan_btn_ = nullptr;
  lv_obj_t* clear_btn_ = nullptr;
  lv_obj_t* content_ = nullptr;

  uint32_t shown_revision_ = 0;
  uint32_t last_summary_ms_ = 0;
  bool awaiting_result_ = false;
};

}  // namespace ui
