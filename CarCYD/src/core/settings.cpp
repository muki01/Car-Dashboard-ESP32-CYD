#include "settings.h"

#include <type_traits>

#include "log.h"
#include "persist.h"
#include "util.h"

namespace core::settings {
namespace {

static_assert(std::is_trivially_copyable<Settings>::value, "Settings must stay trivially copyable");

constexpr uint32_t kSaveDelayMs = 1500;
constexpr uint8_t kMaxListeners = 8;

struct ListenerSlot {
  Listener fn;
  void* user;
};

Settings g_settings = defaults();
Storage* g_storage = nullptr;
ListenerSlot g_listeners[kMaxListeners];
uint8_t g_listener_count = 0;
bool g_dirty = false;
uint32_t g_dirty_since = 0;
bool g_dirty_timer_running = false;

template <typename E>
E sanitize_enum(E value, E count, E fallback) {
  return static_cast<uint8_t>(value) < static_cast<uint8_t>(count) ? value : fallback;
}

uint16_t round_to(uint16_t v, uint16_t step) { return static_cast<uint16_t>((v + step / 2) / step * step); }

void sanitize(Settings& s) {
  const Settings d = defaults();
  s.brightness = clamp<uint8_t>(s.brightness, 5, 100);
  s.night_brightness = clamp<uint8_t>(s.night_brightness, 5, 100);
  s.accent = sanitize_enum(s.accent, Accent::Count, d.accent);
  s.language = sanitize_enum(s.language, Language::Count, d.language);
  s.speed_unit = sanitize_enum(s.speed_unit, SpeedUnit::Count, d.speed_unit);
  s.temp_unit = sanitize_enum(s.temp_unit, TempUnit::Count, d.temp_unit);
  s.pressure_unit = sanitize_enum(s.pressure_unit, PressureUnit::Count, d.pressure_unit);

  s.rpm_max = clamp<uint16_t>(round_to(s.rpm_max, 1000), 4000, 10000);
  s.redline_rpm = clamp<uint16_t>(round_to(s.redline_rpm, 250), 2000, s.rpm_max);
  s.shift_rpm = clamp<uint16_t>(round_to(s.shift_rpm, 100), 1500, s.rpm_max);

  s.coolant_warn_c = clamp<uint8_t>(s.coolant_warn_c, 90, 125);
  s.low_voltage_dv = clamp<uint8_t>(s.low_voltage_dv, 105, 125);
  s.fuel_warn_pct = clamp<uint8_t>(s.fuel_warn_pct, 0, 30);
  if (s.overspeed_kmh != 0) s.overspeed_kmh = clamp<uint16_t>(s.overspeed_kmh, 30, 250);
  s.volume = clamp<uint8_t>(s.volume, 0, 100);
  s.dash_view = clamp<uint8_t>(s.dash_view, 0, 1);

  for (auto& slot : s.corner_slots) slot = sanitize_enum(slot, SignalId::Count, SignalId::CoolantTemp);
  for (auto& slot : s.gauge_slots) slot = sanitize_enum(slot, SignalId::Count, SignalId::EngineLoad);
}

void notify(uint32_t changes) {
  for (uint8_t i = 0; i < g_listener_count; ++i) g_listeners[i].fn(changes, g_listeners[i].user);
}

}  // namespace

Settings defaults() {
  Settings s{};
  s.brightness = 85;
  s.night_brightness = 25;
  s.auto_brightness = true;
  s.flip_screen = false;
  s.accent = Accent::Cyan;
  s.language = Language::English;
  s.speed_unit = SpeedUnit::Kmh;
  s.temp_unit = TempUnit::Celsius;
  s.pressure_unit = PressureUnit::Bar;
  s.rpm_max = 8000;
  s.redline_rpm = 6500;
  s.shift_light = true;
  s.shift_rpm = 6000;
  s.coolant_warn_c = 105;
  s.low_voltage_dv = 118;
  s.fuel_warn_pct = 10;
  s.overspeed_kmh = 0;
  s.touch_sound = true;
  s.alert_sound = true;
  s.volume = 60;
  s.status_led = true;
  s.demo_mode = true;
  s.dash_view = 0;
  s.corner_slots[0] = SignalId::CoolantTemp;
  s.corner_slots[1] = SignalId::BatteryVoltage;
  s.corner_slots[2] = SignalId::FuelLevel;
  s.corner_slots[3] = SignalId::OilTemp;
  s.gauge_slots[0] = SignalId::EngineLoad;
  s.gauge_slots[1] = SignalId::ThrottlePos;
  s.gauge_slots[2] = SignalId::IntakePressure;
  s.gauge_slots[3] = SignalId::IntakeTemp;
  s.gauge_slots[4] = SignalId::TimingAdvance;
  s.gauge_slots[5] = SignalId::FuelRate;
  return s;
}

const Settings& get() { return g_settings; }

Settings& edit() { return g_settings; }

void commit(uint32_t changes) {
  sanitize(g_settings);
  g_dirty = true;
  g_dirty_timer_running = false;  // restart the save delay
  notify(changes);
}

bool add_listener(Listener listener, void* user) {
  if (listener == nullptr || g_listener_count >= kMaxListeners) return false;
  g_listeners[g_listener_count++] = {listener, user};
  return true;
}

bool load(Storage& storage) {
  g_storage = &storage;
  Settings loaded = defaults();
  const persist::LoadResult result = persist::load(storage, storage_key::kSettings, &loaded, sizeof(loaded));
  switch (result) {
    case persist::LoadResult::Ok:
    case persist::LoadResult::Upgraded:
      sanitize(loaded);
      g_settings = loaded;
      LOG_I("CFG", "settings loaded%s", result == persist::LoadResult::Upgraded ? " (upgraded layout)" : "");
      return true;
    case persist::LoadResult::Missing:
      LOG_I("CFG", "no stored settings, using defaults");
      break;
    case persist::LoadResult::Corrupt:
      LOG_W("CFG", "stored settings corrupt, using defaults");
      break;
  }
  g_settings = defaults();
  return false;
}

void flush() {
  if (!g_dirty || g_storage == nullptr) return;
  if (persist::save(*g_storage, storage_key::kSettings, kSettingsVersion, &g_settings, sizeof(g_settings))) {
    LOG_D("CFG", "settings saved");
  }
  g_dirty = false;
  g_dirty_timer_running = false;
}

void service(uint32_t now_ms) {
  if (!g_dirty) return;
  if (!g_dirty_timer_running) {
    g_dirty_timer_running = true;
    g_dirty_since = now_ms;
    return;
  }
  if (elapsed(now_ms, g_dirty_since) >= kSaveDelayMs) flush();
}

void restore_defaults() {
  g_settings = defaults();
  commit(kChangeAll);
}

}  // namespace core::settings
