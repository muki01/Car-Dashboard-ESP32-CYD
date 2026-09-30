/**
 * @file settings_page.h
 * Settings: category list with one sub-page per category.
 */
#pragma once

#include "page.h"

namespace ui {

class SettingsPage final : public Page {
 public:
  enum class Section : int8_t { None = -1, Display, Units, Gauges, Warnings, Sound, Connection, System, About, Count };

  ~SettingsPage() override;
  void create(lv_obj_t* parent) override;
  const char* title() const override;
  void on_show() override;
  void update(uint32_t now_ms) override;
  bool on_back() override;
  void on_settings_changed(uint32_t changes) override;
  int save_state() const override { return static_cast<int>(section_); }
  void restore_state(int state) override;

 private:
  void build_master();
  void refresh_summaries();
  void open(Section section);
  void close_section();

  void build_display(lv_obj_t* list);
  void build_units(lv_obj_t* list);
  void build_gauges(lv_obj_t* list);
  void build_warnings(lv_obj_t* list);
  void build_sound(lv_obj_t* list);
  void build_connection(lv_obj_t* list);
  void build_system(lv_obj_t* list);
  void build_about(lv_obj_t* list);
  void update_live_values(uint32_t now_ms);

  static void category_cb(lv_event_t* e);

  lv_obj_t* master_ = nullptr;
  lv_obj_t* summaries_[static_cast<int>(Section::Count)] = {};
  lv_obj_t* detail_ = nullptr;
  Section section_ = Section::None;

  // Live values in sub-pages (null when not shown).
  lv_obj_t* ambient_label_ = nullptr;
  lv_obj_t* night_row_ = nullptr;
  lv_obj_t* day_title_ = nullptr;
  lv_obj_t* brightness_value_ = nullptr;
  lv_obj_t* night_value_ = nullptr;
  lv_obj_t* volume_value_ = nullptr;
  lv_obj_t* link_status_ = nullptr;
  lv_obj_t* memory_label_ = nullptr;
  lv_obj_t* uptime_label_ = nullptr;
  lv_obj_t* accent_swatches_[5] = {};
  uint16_t built_rpm_max_ = 0;
  uint32_t last_live_ms_ = 0;
};

}  // namespace ui
