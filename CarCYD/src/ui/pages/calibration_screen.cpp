#include "calibration_screen.h"

#include <math.h>
#include <stdio.h>

#include "../../core/i18n.h"
#include "../../core/log.h"
#include "../../core/platform.h"
#include "../../core/touch_calibration.h"
#include "../theme.h"
#include "../widgets.h"

namespace ui::calibration {
namespace {

// Reference targets (10 % margins, spread as a triangle) and the verification target.
constexpr core::TouchPoint kTargets[] = {{32, 24}, {288, 120}, {160, 216}, {96, 150}};
constexpr uint8_t kReferenceCount = 3;
constexpr uint8_t kTargetCount = 4;

constexpr uint32_t kPollMs = 20;
constexpr uint8_t kSamplesNeeded = 12;
constexpr uint8_t kReleaseReads = 4;
constexpr int32_t kMaxErrorPx = 16;  // well below the 40 px minimum touch target
constexpr uint32_t kResultShowMs = 1400;

enum class State : uint8_t { WaitPress, Sampling, WaitRelease, Result };

struct Context {
  lv_obj_t* screen;
  lv_obj_t* target;
  lv_obj_t* ring;
  lv_obj_t* step;
  lv_obj_t* instruction;
  lv_timer_t* timer;
  DoneFn on_done;
  State state;
  uint8_t index;
  uint8_t samples;
  uint8_t released_reads;
  int32_t sum_x;
  int32_t sum_y;
  core::TouchPoint raw[kTargetCount];
  core::TouchCalibration result;
  bool success;
  uint32_t result_since;
};

Context g;

void place_target() {
  const core::TouchPoint& p = kTargets[g.index];
  lv_obj_set_pos(g.target, p.x - 20, p.y - 20);
  lv_obj_set_style_bg_opa(g.ring, LV_OPA_TRANSP, 0);

  char buf[32];
  snprintf(buf, sizeof(buf), core::tr(core::Str::CAL_FMT_STEP), static_cast<unsigned>(g.index + 1),
           static_cast<unsigned>(kTargetCount));
  lv_label_set_text(g.step, buf);
  lv_label_set_text(g.instruction,
                    core::tr(g.index < kReferenceCount ? core::Str::CAL_TAP : core::Str::CAL_VERIFY));
  lv_obj_set_style_text_color(g.instruction, color::text(), 0);
}

void restart_sequence() {
  g.index = 0;
  g.state = State::WaitPress;
  set_visible(g.target, true);
  place_target();
}

void finish_point() {
  g.raw[g.index] = {g.sum_x / g.samples, g.sum_y / g.samples};
  platform::play(platform::Sound::Click);

  if (g.index == kReferenceCount - 1) {
    core::TouchPoint screen[kReferenceCount];
    for (uint8_t i = 0; i < kReferenceCount; ++i) screen[i] = kTargets[i];
    if (!core::TouchCalibration::solve(g.raw, screen, g.result)) {
      g.success = false;
      g.state = State::Result;
      return;
    }
  }

  if (g.index == kTargetCount - 1) {
    const core::TouchPoint mapped = g.result.map(g.raw[g.index].x, g.raw[g.index].y);
    const int32_t dx = mapped.x - kTargets[g.index].x;
    const int32_t dy = mapped.y - kTargets[g.index].y;
    const int32_t error = static_cast<int32_t>(sqrtf(static_cast<float>(dx * dx + dy * dy)));
    LOG_I("CAL", "verification error %ld px", static_cast<long>(error));
    g.success = error <= kMaxErrorPx;
    g.state = State::Result;
    return;
  }

  g.index++;
  g.state = State::WaitRelease;
  g.released_reads = 0;
}

void show_result() {
  set_visible(g.target, false);
  lv_label_set_text(g.step, "");
  if (g.success) {
    platform::touch_apply_calibration(g.result);
    platform::play(platform::Sound::Confirm);
    lv_label_set_text(g.instruction, core::tr(core::Str::CAL_DONE));
    lv_obj_set_style_text_color(g.instruction, color::ok(), 0);
  } else {
    platform::play(platform::Sound::Error);
    lv_label_set_text(g.instruction, core::tr(core::Str::CAL_FAILED));
    lv_obj_set_style_text_color(g.instruction, color::warn(), 0);
  }
  g.result_since = platform::millis();
}

void timer_cb(lv_timer_t*) {
  platform::TouchSample sample{};
  const bool pressed = platform::touch_read_raw(sample) && sample.pressed;

  switch (g.state) {
    case State::WaitPress:
      if (pressed) {
        g.samples = 0;
        g.sum_x = g.sum_y = 0;
        g.state = State::Sampling;
        lv_obj_set_style_bg_opa(g.ring, LV_OPA_COVER, 0);
      }
      break;

    case State::Sampling:
      if (!pressed) {
        // Lifted too early: start this point again.
        lv_obj_set_style_bg_opa(g.ring, LV_OPA_TRANSP, 0);
        g.state = State::WaitPress;
        break;
      }
      g.sum_x += sample.raw_x;
      g.sum_y += sample.raw_y;
      if (++g.samples >= kSamplesNeeded) {
        finish_point();
        if (g.state == State::Result) show_result();
      }
      break;

    case State::WaitRelease:
      g.released_reads = pressed ? 0 : g.released_reads + 1;
      if (g.released_reads >= kReleaseReads) {
        place_target();
        g.state = State::WaitPress;
      }
      break;

    case State::Result:
      if (platform::millis() - g.result_since < kResultShowMs || pressed) break;
      if (g.success) {
        lv_timer_delete(g.timer);
        g.timer = nullptr;
        platform::touch_input_enable(true);
        if (g.on_done != nullptr) g.on_done();
        lv_obj_delete_async(g.screen);
      } else {
        restart_sequence();
      }
      break;
  }
}

}  // namespace

void start(DoneFn on_done) {
  g = Context{};
  g.on_done = on_done;
  platform::touch_input_enable(false);

  g.screen = lv_obj_create(nullptr);
  lv_obj_remove_style_all(g.screen);
  lv_obj_set_style_bg_color(g.screen, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(g.screen, LV_OPA_COVER, 0);
  lv_obj_set_scrollable(g.screen, false);

  lv_obj_t* title = label(g.screen, font::title(), color::text(), core::tr(core::Str::CAL_TITLE));
  lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 44);
  g.step = label(g.screen, font::caption(), color::text_dim(), "");
  lv_obj_align(g.step, LV_ALIGN_TOP_MID, 0, 64);
  g.instruction = label(g.screen, font::body(), color::text(), "");
  lv_obj_align(g.instruction, LV_ALIGN_CENTER, 0, -18);
  lv_obj_t* hint = label(g.screen, font::caption(), color::text_faint(), core::tr(core::Str::CAL_HINT));
  lv_obj_align(hint, LV_ALIGN_CENTER, 0, 64);

  // Crosshair target: 40 x 40 box with a ring and two hairlines.
  g.target = box(g.screen);
  lv_obj_set_size(g.target, 40, 40);
  g.ring = box(g.target);
  lv_obj_set_size(g.ring, 22, 22);
  lv_obj_center(g.ring);
  lv_obj_set_style_radius(g.ring, LV_RADIUS_CIRCLE, 0);
  lv_obj_set_style_border_width(g.ring, 2, 0);
  lv_obj_set_style_border_color(g.ring, color::accent(), 0);
  lv_obj_set_style_bg_color(g.ring, color::accent(), 0);
  lv_obj_t* h = box(g.target);
  lv_obj_set_size(h, 40, 2);
  lv_obj_center(h);
  lv_obj_set_style_bg_color(h, color::text(), 0);
  lv_obj_set_style_bg_opa(h, LV_OPA_COVER, 0);
  lv_obj_t* v = box(g.target);
  lv_obj_set_size(v, 2, 40);
  lv_obj_center(v);
  lv_obj_set_style_bg_color(v, color::text(), 0);
  lv_obj_set_style_bg_opa(v, LV_OPA_COVER, 0);

  restart_sequence();
  lv_screen_load(g.screen);
  g.timer = lv_timer_create(timer_cb, kPollMs, nullptr);
  LOG_I("CAL", "touch calibration started");
}

}  // namespace ui::calibration
