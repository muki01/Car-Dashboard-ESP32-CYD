#include "buzzer.h"

#include <Arduino.h>

#include "../config/board_config.h"
#include "../core/util.h"

namespace hal::buzzer {
namespace {

struct Step {
  uint16_t hz;  // 0 = pause
  uint16_t ms;
};

struct Melody {
  const Step* steps;
  uint8_t count;
  uint8_t priority;
};

// clang-format off
constexpr Step kClick[]    = {{4200, 8}};
constexpr Step kConfirm[]  = {{1760, 55}, {0, 25}, {2637, 90}};
constexpr Step kError[]    = {{880, 110}, {0, 40}, {622, 180}};
constexpr Step kStartup[]  = {{1319, 70}, {0, 15}, {1760, 70}, {0, 15}, {2637, 140}};
constexpr Step kWarning[]  = {{2200, 130}, {0, 90}, {2200, 130}};
constexpr Step kCritical[] = {{2900, 160}, {0, 50}, {2150, 160}, {0, 50}, {2900, 160}, {0, 50}, {2150, 160}};
// clang-format on

constexpr Melody kMelodies[] = {
    {kClick, 1, 0},   {kConfirm, 3, 1}, {kError, 3, 1},
    {kStartup, 5, 1}, {kWarning, 3, 2}, {kCritical, 7, 3},
};

constexpr uint32_t kIdleFrequency = 2000;
constexpr uint8_t kPwmBits = 10;
constexpr uint32_t kMaxDuty = 1u << (kPwmBits - 1);  // 50 % = loudest square wave

const Melody* g_melody = nullptr;
uint8_t g_step = 0;
uint32_t g_step_end = 0;
uint8_t g_volume = 60;

void output(uint16_t hz) {
  if (hz == 0 || g_volume == 0) {
    ledcWrite(board::kSpeaker, 0);
    return;
  }
  ledcWriteTone(board::kSpeaker, hz);
  // Lower duty = less energy in the fundamental = quieter. Quadratic feels linear.
  const uint32_t duty = kMaxDuty * g_volume * g_volume / 10000;
  ledcWrite(board::kSpeaker, duty > 0 ? duty : 1);
}

void start_step(uint32_t now_ms) {
  const Step& s = g_melody->steps[g_step];
  output(s.hz);
  g_step_end = now_ms + s.ms;
}

}  // namespace

void init() {
  // Unique frequency/resolution so the speaker gets its own LEDC timer.
  ledcAttachChannel(board::kSpeaker, kIdleFrequency, kPwmBits, board::kLedcSpeaker);
  ledcWrite(board::kSpeaker, 0);
}

void set_volume(uint8_t percent) { g_volume = core::clamp<uint8_t>(percent, 0, 100); }

void play(Sound sound) {
  const Melody& m = kMelodies[static_cast<uint8_t>(sound)];
  if (g_melody != nullptr && m.priority < g_melody->priority) return;
  g_melody = &m;
  g_step = 0;
  start_step(millis());
}

void stop() {
  g_melody = nullptr;
  output(0);
}

void update(uint32_t now_ms) {
  if (g_melody == nullptr || static_cast<int32_t>(now_ms - g_step_end) < 0) return;
  if (++g_step >= g_melody->count) {
    stop();
    return;
  }
  start_step(now_ms);
}

}  // namespace hal::buzzer
