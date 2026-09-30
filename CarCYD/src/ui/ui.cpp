#include "ui.h"

#include <stdio.h>

#include "../config/app_config.h"
#include "../core/alerts.h"
#include "../core/i18n.h"
#include "../core/log.h"
#include "../core/platform.h"
#include "../core/settings.h"
#include "../core/signals.h"
#include "../core/units.h"
#include "../core/vehicle_state.h"
#include "components/banner.h"
#include "components/dialog.h"
#include "components/nav_rail.h"
#include "components/status_bar.h"
#include "pages/calibration_screen.h"
#include "pages/dashboard_page.h"
#include "pages/diag_page.h"
#include "pages/live_page.h"
#include "pages/settings_page.h"
#include "pages/splash_screen.h"
#include "pages/trip_page.h"
#include "theme.h"
#include "widgets.h"

namespace ui {
namespace {

constexpr int kPageCount = static_cast<int>(PageId::Count);
constexpr uint32_t kToastMs = 2500;

struct Shell {
  lv_display_t* display = nullptr;
  lv_obj_t* screen = nullptr;
  lv_obj_t* content = nullptr;
  StatusBar status;
  NavRail rail;
  Page* pages[kPageCount] = {};
  PageId current = PageId::Dashboard;
  lv_timer_t* timer = nullptr;
  uint32_t alert_revision = 0;
  uint32_t dtc_revision = 0;
  bool started = false;
  bool rebuild_pending = false;
};

Shell g;

Page* make_page(PageId id) {
  switch (id) {
    case PageId::Dashboard: return new DashboardPage();
    case PageId::Live: return new LivePage();
    case PageId::Diagnostics: return new DiagPage();
    case PageId::Trip: return new TripPage();
    case PageId::Settings:
    default: return new SettingsPage();
  }
}

Page* page(PageId id) {
  Page*& p = g.pages[static_cast<int>(id)];
  if (p == nullptr) {
    p = make_page(id);
    p->create(g.content);
    set_visible(p->root(), false);
  }
  return p;
}

// ---- Alerts -> banner ----------------------------------------------------------------------

void alert_tapped(void* user) {
  const auto id = static_cast<core::AlertId>(reinterpret_cast<intptr_t>(user));
  core::alerts().acknowledge(id);
  if (id == core::AlertId::CheckEngine) navigate(PageId::Diagnostics);
}

void refresh_alert_banner(uint32_t now_ms) {
  if (banner::visible() && banner::owner() < 0) return;  // let a toast finish first

  core::AlertManager& alerts = core::alerts();
  const core::AlertId top = alerts.top_unacknowledged();
  if (top == core::AlertId::Count) {
    if (banner::visible()) banner::hide();
    return;
  }
  const bool showing = banner::visible() && banner::owner() == static_cast<int>(top);
  if (showing && alerts.revision() == g.alert_revision) return;
  g.alert_revision = alerts.revision();

  // Compose "Engine temperature high" + "112 °C · tap to dismiss".
  char text[64];
  const core::SignalId sig = core::alert_signal(top);
  if (sig != core::SignalId::Count && core::vehicle().fresh(sig, now_ms)) {
    char value[16];
    const core::Settings& s = core::settings::get();
    core::units::format_value(sig, core::vehicle().get(sig).value, s, value, sizeof(value));
    snprintf(text, sizeof(text), "%s %s · %s", value, core::units::label(core::signal_info(sig).quantity, s),
             core::tr(core::Str::ALERT_TAP_HINT));
  } else if (top == core::AlertId::CheckEngine) {
    snprintf(text, sizeof(text), "%s", core::tr(core::Str::ALERT_MIL_TEXT));
  } else if (top == core::AlertId::LinkLost) {
    snprintf(text, sizeof(text), "%s", core::tr(core::Str::ALERT_LINK_TEXT));
  } else {
    snprintf(text, sizeof(text), "%s", core::tr(core::Str::ALERT_TAP_HINT));
  }

  const core::Alert& alert = alerts.get(top);
  const char* glyph = top == core::AlertId::CheckEngine ? ICON_ENGINE : ICON_ALERT;
  banner::show(alert.severity, glyph, core::tr(core::alert_title(top)), text, 0, alert_tapped,
               reinterpret_cast<void*>(static_cast<intptr_t>(top)));
  banner::set_owner(static_cast<int>(top));
}

// ---- Periodic refresh ------------------------------------------------------------------------

void tick_cb(lv_timer_t*) {
  const uint32_t now = platform::millis();
  g.status.update(now);
  banner::tick(now);
  refresh_alert_banner(now);

  const core::DtcState& dtc = core::vehicle().dtc;
  if (dtc.revision != g.dtc_revision) {
    g.dtc_revision = dtc.revision;
    const bool attention = dtc.mil_known && dtc.mil_on;
    g.rail.set_badge(PageId::Diagnostics, attention || dtc.count > 0, attention ? color::warn() : color::info());
  }

  g.pages[static_cast<int>(g.current)]->update(now);
}

// ---- Construction ----------------------------------------------------------------------------

void back_cb(lv_event_t*) {
  Page* p = g.pages[static_cast<int>(g.current)];
  if (p != nullptr) p->on_back();
}

void rail_select(PageId id) { navigate(id); }

void build_shell() {
  g.screen = lv_obj_create(nullptr);
  lv_obj_remove_style_all(g.screen);
  lv_obj_set_style_bg_color(g.screen, color::bg(), 0);
  lv_obj_set_style_bg_opa(g.screen, LV_OPA_COVER, 0);
  lv_obj_set_scrollable(g.screen, false);

  g.content = box(g.screen);
  lv_obj_set_pos(g.content, layout::kRailW, layout::kStatusH);
  lv_obj_set_size(g.content, layout::kContentW, layout::kContentH);

  g.rail.create(g.screen, rail_select);
  g.status.create(g.screen, back_cb, nullptr);
  g.alert_revision = core::alerts().revision() - 1;
  g.dtc_revision = core::vehicle().dtc.revision - 1;
}

void settings_changed(uint32_t changes, void*) {
  if (changes & core::settings::kChangeLanguage) core::i18n::set_language(core::settings::get().language);
  if (!g.started) return;
  if (changes & (core::settings::kChangeLanguage | core::settings::kChangeTheme)) {
    request_rebuild();
    return;
  }
  for (Page* p : g.pages) {
    if (p != nullptr) p->on_settings_changed(changes);
  }
  if (changes & core::settings::kChangeUnits) g.alert_revision--;  // re-render banner text
}

void rebuild_async(void*) {
  g.rebuild_pending = false;
  const PageId current = g.current;
  const int state = g.pages[static_cast<int>(current)] ? g.pages[static_cast<int>(current)]->save_state() : -1;

  theme_apply(g.display);
  lv_obj_t* old_screen = g.screen;
  for (auto& p : g.pages) {
    delete p;
    p = nullptr;
  }
  dialog::close();
  build_shell();
  lv_screen_load(g.screen);
  if (old_screen != nullptr) lv_obj_delete(old_screen);

  // The banner lives on the top layer and caches colours: recreate it.
  banner::deinit();
  banner::init();

  navigate(current);
  if (state >= 0) g.pages[static_cast<int>(current)]->restore_state(state);
  LOG_I("UI", "rebuilt");
}

void calibration_done() {
  lv_screen_load(g.screen);
  g.pages[static_cast<int>(g.current)]->on_show();
}

}  // namespace

void init(lv_display_t* display) {
  g.display = display;
  core::i18n::set_language(core::settings::get().language);
  theme_apply(display);
  core::settings::add_listener(settings_changed, nullptr);
}

void splash_show() { splash::create(); }

void splash_progress(uint8_t percent, const char* text) { splash::set_progress(percent, text); }

void start(bool run_touch_calibration) {
  lv_obj_t* splash = lv_screen_active();
  build_shell();
  banner::init();
  navigate(PageId::Dashboard);
  g.timer = lv_timer_create(tick_cb, APP_UI_TICK_MS, nullptr);
  g.started = true;

  if (run_touch_calibration) {
    calibration::start(calibration_done);
    lv_obj_delete(splash);
  } else {
    lv_screen_load_anim(g.screen, LV_SCREEN_LOAD_ANIM_FADE_IN, motion::kNormal, 0, true);
  }
}

void navigate(PageId id) {
  if (id >= PageId::Count || g.content == nullptr) return;
  Page* previous = g.pages[static_cast<int>(g.current)];
  if (previous != nullptr && g.current != id) {
    previous->on_back();  // leave sub-views
    previous->on_hide();
    set_visible(previous->root(), false);
  }
  g.current = id;
  Page* p = page(id);
  set_visible(p->root(), true);
  g.rail.set_active(id);
  g.status.set_title(p->title());
  g.status.show_back(false);
  p->on_show();
  p->update(platform::millis());
}

PageId current_page() { return g.current; }

void open_touch_calibration() { calibration::start(calibration_done); }

void request_rebuild() {
  if (g.rebuild_pending) return;
  g.rebuild_pending = true;
  lv_async_call(rebuild_async, nullptr);
}

void set_title(const char* title) { g.status.set_title(title); }

void show_back_button(bool show) { g.status.show_back(show); }

void toast(core::Severity severity, const char* title, const char* text) {
  const char* glyph = severity == core::Severity::Normal     ? ICON_CHECK_CIRCLE
                      : severity == core::Severity::Critical ? ICON_ALERT_CIRCLE
                                                             : ICON_INFO;
  banner::show(severity, glyph, title, text, kToastMs, nullptr, nullptr);
}

void on_hardware_button_short() {
  if (dialog::is_open()) {
    dialog::close();
    return;
  }
  const core::AlertId top = core::alerts().top_unacknowledged();
  if (top != core::AlertId::Count) {
    core::alerts().acknowledge(top);
    return;
  }
  navigate(static_cast<PageId>((static_cast<int>(g.current) + 1) % kPageCount));
}

}  // namespace ui
