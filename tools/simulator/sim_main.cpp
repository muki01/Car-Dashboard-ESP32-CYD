/**
 * @file sim_main.cpp
 * Headless desktop simulator of the CarCYD UI.
 *
 * Runs the real core + UI code against LVGL on the PC with a virtual clock and
 * scripted touch input, and writes screenshots (PPM) of every screen. Used to
 * review the design without hardware and as a UI smoke test.
 *
 * Usage: carcyd_sim <output dir> [tr|en]
 */
#include <lvgl.h>
#include <stdio.h>
#include <string.h>

#include <map>
#include <string>
#include <vector>

#include "../../CarCYD/src/core/alerts.h"
#include "../../CarCYD/src/core/data_link.h"
#include "../../CarCYD/src/core/i18n.h"
#include "../../CarCYD/src/core/log.h"
#include "../../CarCYD/src/core/settings.h"
#include "../../CarCYD/src/core/sim_link.h"
#include "../../CarCYD/src/core/trip.h"
#include "../../CarCYD/src/core/vehicle_state.h"
#include "../../CarCYD/src/ui/ui.h"
#include "sim.h"

namespace sim {
namespace {

constexpr int32_t kW = 320;
constexpr int32_t kH = 240;
constexpr uint32_t kStepMs = 5;

uint32_t g_now = 1;
bool g_pressed = false;
bool g_input = true;
int32_t g_x = 0;
int32_t g_y = 0;
uint16_t g_fb[kW * kH];
std::string g_out_dir = "screenshots";
core::SimLink g_link;
int g_shots = 0;

// Animation recording (demo GIF): one frame every kFrameMs while recording.
constexpr uint32_t kFrameMs = 100;
bool g_recording = false;
bool g_auto_ack = false;  // acknowledge alerts silently (clean demo footage)
int g_frames = 0;
uint32_t g_next_frame_ms = 0;
FILE* g_manifest = nullptr;

class MemoryStorage final : public core::Storage {
 public:
  bool read(const char* key, void* data, size_t len) override {
    auto it = data_.find(key);
    if (it == data_.end() || it->second.size() != len) return false;
    memcpy(data, it->second.data(), len);
    return true;
  }
  size_t size(const char* key) override {
    auto it = data_.find(key);
    return it == data_.end() ? 0 : it->second.size();
  }
  bool write(const char* key, const void* data, size_t len) override {
    const auto* p = static_cast<const uint8_t*>(data);
    data_[key] = std::vector<uint8_t>(p, p + len);
    return true;
  }
  void erase_all() override { data_.clear(); }

 private:
  std::map<std::string, std::vector<uint8_t>> data_;
};

MemoryStorage g_storage;

void flush_cb(lv_display_t* disp, const lv_area_t* area, uint8_t* px) {
  const int32_t w = lv_area_get_width(area);
  for (int32_t y = area->y1; y <= area->y2; ++y) {
    memcpy(&g_fb[y * kW + area->x1], px + (y - area->y1) * w * 2, w * 2);
  }
  lv_display_flush_ready(disp);
}

void read_cb(lv_indev_t*, lv_indev_data_t* data) {
  data->point.x = g_x;
  data->point.y = g_y;
  data->state = (g_pressed && g_input) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

uint32_t tick_cb() { return g_now; }

void log_sink(const char* line) { printf("%s\n", line); }

void write_ppm(const char* path) {
  FILE* f = fopen(path, "wb");
  if (f == nullptr) {
    printf("[sim] cannot write %s\n", path);
    return;
  }
  fprintf(f, "P6\n%d %d\n255\n", kW, kH);
  for (int32_t i = 0; i < kW * kH; ++i) {
    const uint16_t v = static_cast<uint16_t>((g_fb[i] >> 8) | (g_fb[i] << 8));  // big-endian RGB565
    const uint8_t rgb[3] = {static_cast<uint8_t>(((v >> 11) & 0x1F) * 255 / 31),
                            static_cast<uint8_t>(((v >> 5) & 0x3F) * 255 / 63),
                            static_cast<uint8_t>((v & 0x1F) * 255 / 31)};
    fwrite(rgb, 1, 3, f);
  }
  fclose(f);
}

void record_frame() {
  char path[512];
  snprintf(path, sizeof(path), "%s/frame_%04d.ppm", g_out_dir.c_str(), g_frames);
  write_ppm(path);
  if (g_manifest != nullptr) fprintf(g_manifest, "%d %d %d %d\n", g_frames, g_pressed ? 1 : 0, g_x, g_y);
  g_frames++;
}

void loop_once() {
  core::link::poll(g_now);
  core::alerts().update(core::vehicle(), core::settings::get(), g_now);
  if (g_auto_ack) {
    const core::AlertId top = core::alerts().top_unacknowledged();
    if (top != core::AlertId::Count) core::alerts().acknowledge(top);
  }
  core::trip().update(core::vehicle(), core::settings::get(), g_now);
  core::settings::service(g_now);
  lv_timer_handler();
}

}  // namespace

uint32_t now() { return g_now; }
bool touch_pressed() { return g_pressed; }
int32_t touch_x() { return g_x; }
int32_t touch_y() { return g_y; }
void set_touch_input(bool enabled) { g_input = enabled; }

// ---- Script helpers ---------------------------------------------------------------------

void run(uint32_t ms) {
  for (uint32_t t = 0; t < ms; t += kStepMs) {
    g_now += kStepMs;
    loop_once();
    if (g_recording && g_now >= g_next_frame_ms) {
      record_frame();
      g_next_frame_ms += kFrameMs;
    }
  }
}

void record(bool on) {
  if (on && !g_recording) g_next_frame_ms = g_now;
  g_recording = on;
}

void shot(const char* name) {
  lv_refr_now(nullptr);
  char path[512];
  snprintf(path, sizeof(path), "%s/%s.ppm", g_out_dir.c_str(), name);
  write_ppm(path);
  g_shots++;
  printf("[sim] t=%6.1fs  %s\n", g_now / 1000.0, name);
}

void press(int32_t x, int32_t y) {
  g_x = x;
  g_y = y;
  g_pressed = true;
}

void release() { g_pressed = false; }

void tap(int32_t x, int32_t y) {
  press(x, y);
  run(120);
  release();
  run(150);
}

void long_press(int32_t x, int32_t y) {
  press(x, y);
  run(900);
  release();
  run(200);
}

void swipe(int32_t x0, int32_t y0, int32_t x1, int32_t y1, uint32_t duration_ms = 200) {
  press(x0, y0);
  run(40);
  const int steps = static_cast<int>(duration_ms / 20);
  for (int i = 1; i <= steps; ++i) {
    g_x = x0 + (x1 - x0) * i / steps;
    g_y = y0 + (y1 - y0) * i / steps;
    run(20);
  }
  release();
  run(400);
}

// Screen geometry helpers (see ui.h).
constexpr int32_t kRailX = 28;
int32_t rail_y(int index) { return 24 + index * 48; }

}  // namespace sim

// Scenario ------------------------------------------------------------------------------------

namespace {

using core::SignalId;

std::string g_lang = "tr";

void snap(const char* base) {
  char name[64];
  snprintf(name, sizeof(name), "%s_%s", g_lang.c_str(), base);
  sim::shot(name);
}

float value(SignalId id) { return core::vehicle().value_or(id, sim::now(), 0.0f); }

template <typename Pred>
bool run_until(Pred done, uint32_t timeout_ms) {
  for (uint32_t t = 0; t < timeout_ms; t += 20) {
    sim::run(20);
    if (done()) return true;
  }
  printf("[sim] condition not reached within %lu ms\n", static_cast<unsigned long>(timeout_ms));
  return false;
}

void back() {
  sim::tap(79, 14);
  sim::run(250);
}

void wait_toast() { sim::run(3000); }

void settings_row(int y) {
  sim::tap(200, y);
  sim::run(350);
}

void scenario() {
  using namespace sim;

  // Start-up: needle sweep, engine start, check engine light reported by the car.
  run(4200);
  snap("06_alert_check_engine");
  ui::on_hardware_button_short();  // BOOT key acknowledges the alert
  run(400);

  // Dashboard while cruising and at the shift point.
  run_until([] { return value(SignalId::Speed) > 55 && value(SignalId::Rpm) > 1500 && value(SignalId::Rpm) < 3200; },
            120000);
  run(300);
  snap("02_dashboard");
  if (run_until([] { return value(SignalId::Rpm) >= 6050; }, 240000)) {
    run(60);
    snap("03_dashboard_shift");
  }

  // Second dashboard view and gauge customisation.
  swipe(250, 120, 90, 120);
  run(800);
  snap("04_dashboard_engine");
  long_press(100, 80);
  run(300);
  snap("05_signal_picker");
  tap(180, 4);  // backdrop outside the sheet (harmless status bar area otherwise)
  run(300);
  swipe(90, 120, 250, 120);
  run(300);

  // A driver warning: speed limit set to 50 km/h.
  core::settings::edit().overspeed_kmh = 50;
  core::settings::commit(core::settings::kChangeWarnings);
  if (run_until([] { return core::alerts().get(core::AlertId::Overspeed).active(); }, 120000)) {
    run(400);
    snap("07_alert_overspeed");
  }
  core::settings::edit().overspeed_kmh = 0;
  core::settings::commit(core::settings::kChangeWarnings);
  run(600);

  // Live data.
  tap(kRailX, rail_y(1));
  run(600);
  snap("08_live_data");
  tap(200, 100);  // engine speed
  run(9000);
  snap("09_live_detail");
  back();

  // Diagnostics: read, inspect, clear.
  tap(kRailX, rail_y(2));
  run(500);
  snap("10_diag_ready");
  tap(225, 63);
  run(500);
  snap("11_diag_scanning");
  run(2000);
  wait_toast();
  snap("12_diag_codes");
  tap(180, 139);
  run(300);
  snap("13_dtc_detail");
  tap(180, 4);  // backdrop outside the sheet (harmless status bar area otherwise)
  run(300);
  tap(285, 63);
  run(300);
  snap("14_dtc_clear_confirm");
  tap(228, 168);
  run(2000);
  wait_toast();
  snap("15_diag_cleared");

  // Trip computer.
  tap(kRailX, rail_y(3));
  run(600);
  snap("16_trip");

  // Settings: master list and every section.
  tap(kRailX, rail_y(4));
  run(400);
  snap("17_settings");
  const char* upper[] = {"18_settings_display", "19_settings_units", "20_settings_gauges", "21_settings_warnings"};
  for (int i = 0; i < 4; ++i) {
    settings_row(28 + 8 + 22 + i * 50);
    snap(upper[i]);
    back();
  }
  swipe(200, 200, 200, 40, 300);
  run(800);
  const char* lower[] = {"22_settings_sound", "23_settings_connection", "24_settings_system", "25_settings_about"};
  for (int i = 0; i < 4; ++i) {
    settings_row(52 + i * 50);
    snap(lower[i]);
    if (i < 3) back();
  }

  // Runtime theme change: the whole UI is rebuilt and the open section restored.
  core::settings::edit().accent = core::Accent::Amber;
  core::settings::commit(core::settings::kChangeTheme);
  run(500);
  snap("27_rebuild_restored_section");
  back();
  tap(kRailX, rail_y(0));
  run(800);
  snap("28_dashboard_accent_amber");

  // Runtime language switch (same rebuild path), then restore the original setup.
  const core::Language original = core::settings::get().language;
  core::settings::edit().language =
      original == core::Language::Turkish ? core::Language::English : core::Language::Turkish;
  core::settings::commit(core::settings::kChangeLanguage);
  run(500);
  snap("29_language_switched");
  core::settings::edit().language = original;
  core::settings::edit().accent = core::Accent::Cyan;
  core::settings::commit(core::settings::kChangeLanguage | core::settings::kChangeTheme);
  run(500);
}

/** Short product tour recorded frame by frame for the README demo animation. */
void demo_tour() {
  using namespace sim;
  g_auto_ack = true;  // keep the footage free of alert banners

  // Screen fade-in and the start-up needle sweep.
  record(true);
  run(1700);
  record(false);

  // Skip the idle time, then record the car pulling away (gear changes).
  run_until([] { return value(SignalId::Speed) > 1.0f; }, 30000);
  record(true);
  run(6000);

  // Second dashboard view.
  swipe(250, 120, 90, 120);
  run(1200);
  record(false);
  swipe(90, 120, 250, 120);

  // Live data, diagnostics scan and settings.
  record(true);
  tap(kRailX, rail_y(1));
  run(1400);
  tap(kRailX, rail_y(2));
  run(500);
  tap(225, 63);
  run(2300);
  tap(kRailX, rail_y(4));
  run(600);
  tap(200, 58);
  run(1200);
  record(false);
}

}  // namespace

int main(int argc, char** argv) {
  if (argc > 1) sim::g_out_dir = argv[1];
  const bool english = argc > 2 && strcmp(argv[2], "en") == 0;
  const bool demo = argc > 3 && strcmp(argv[3], "demo") == 0;  // record frames instead of screenshots
  g_lang = english ? "en" : "tr";

  core::log::init(sim::log_sink, sim::tick_cb, core::log::Level::Info);
  core::settings::load(sim::g_storage);
  core::settings::edit().language = english ? core::Language::English : core::Language::Turkish;
  core::settings::commit(core::settings::kChangeLanguage);

  lv_init();
  lv_tick_set_cb(sim::tick_cb);

  static uint8_t buf[sim::kW * sim::kH * 2];
  lv_display_t* disp = lv_display_create(sim::kW, sim::kH);
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565_SWAPPED);
  lv_display_set_buffers(disp, buf, nullptr, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(disp, sim::flush_cb);

  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, sim::read_cb);

  ui::init(disp);
  ui::splash_show();
  ui::splash_progress(50, core::tr(core::Str::BOOT_HARDWARE));
  sim::run(100);

  if (demo) {
    const std::string manifest = sim::g_out_dir + "/frames.txt";
    sim::g_manifest = fopen(manifest.c_str(), "w");
    core::link::set_active(&sim::g_link, sim::now());
    ui::start(false);
    demo_tour();
    if (sim::g_manifest != nullptr) fclose(sim::g_manifest);
    printf("[sim] %d frames written to %s\n", sim::g_frames, sim::g_out_dir.c_str());
    return 0;
  }

  snap("01_splash");
  core::link::set_active(&sim::g_link, sim::now());
  ui::start(false);
  scenario();

  ui::open_touch_calibration();
  sim::run(300);
  snap("26_touch_calibration");

  printf("[sim] %d screenshots written to %s\n", sim::g_shots, sim::g_out_dir.c_str());
  return 0;
}
