/**
 * @file live_page.h
 * Live data: every signal in a scrollable list; tapping a row opens a detail
 * view with a real-time chart and min / avg / max statistics.
 */
#pragma once

#include "../../core/types.h"
#include "page.h"

namespace ui {

class LivePage final : public Page {
 public:
  void create(lv_obj_t* parent) override;
  const char* title() const override;
  void on_show() override;
  void update(uint32_t now_ms) override;
  bool on_back() override;
  int save_state() const override;
  void restore_state(int state) override;

 private:
  struct Row {
    lv_obj_t* value;
    lv_obj_t* unit;
  };

  void open_detail(core::SignalId id);
  void close_detail();
  void reset_stats();
  void update_list(uint32_t now_ms);
  void update_detail(uint32_t now_ms);

  static void row_clicked_cb(lv_event_t* e);
  static void reset_cb(lv_event_t* e);

  lv_obj_t* list_ = nullptr;
  Row rows_[core::kSignalCount] = {};
  uint32_t last_list_update_ms_ = 0;

  // Detail view
  lv_obj_t* detail_ = nullptr;
  lv_obj_t* detail_value_ = nullptr;
  lv_obj_t* detail_unit_ = nullptr;
  lv_obj_t* stat_min_ = nullptr;
  lv_obj_t* stat_avg_ = nullptr;
  lv_obj_t* stat_max_ = nullptr;
  lv_obj_t* chart_ = nullptr;
  lv_chart_series_t* series_ = nullptr;
  core::SignalId detail_id_ = core::SignalId::Count;
  uint32_t last_sample_ms_ = 0;
  float min_ = 0.0f;
  float max_ = 0.0f;
  double sum_ = 0.0;
  uint32_t samples_ = 0;
  int32_t axis_min_ = 0;
  int32_t axis_max_ = 0;
};

}  // namespace ui
