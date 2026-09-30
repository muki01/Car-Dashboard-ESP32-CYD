#include "data_link.h"

#include "log.h"

namespace core {

// ---- NullLink -------------------------------------------------------------------

void NullLink::begin(VehicleState& state, uint32_t) {
  state.link_state = LinkState::Disconnected;
  state.simulated = false;
  state.clock_valid = false;
  state.invalidate_all();
}

void NullLink::poll(VehicleState&, uint32_t) {}

bool NullLink::request_dtc_read(VehicleState& state, uint32_t) {
  state.dtc.last_result = DtcResult::NotConnected;
  state.dtc.revision++;
  return false;
}

bool NullLink::request_dtc_clear(VehicleState& state, uint32_t now_ms) { return request_dtc_read(state, now_ms); }

// ---- Link manager ---------------------------------------------------------------

namespace link {
namespace {
DataLink* g_active = nullptr;
}  // namespace

void set_active(DataLink* next, uint32_t now_ms) {
  if (next == g_active) return;
  VehicleState& state = vehicle();
  if (g_active != nullptr) g_active->end(state);

  // A new source starts from a clean slate.
  state.invalidate_all();
  state.dtc = DtcState{};
  state.dtc.revision = 1;

  g_active = next;
  if (g_active != nullptr) {
    LOG_I("LINK", "data source: %s", g_active->name());
    g_active->begin(state, now_ms);
  }
}

DataLink* active() { return g_active; }

void poll(uint32_t now_ms) {
  if (g_active != nullptr) g_active->poll(vehicle(), now_ms);
}

bool request_dtc_read(uint32_t now_ms) {
  return g_active != nullptr && g_active->request_dtc_read(vehicle(), now_ms);
}

bool request_dtc_clear(uint32_t now_ms) {
  return g_active != nullptr && g_active->request_dtc_clear(vehicle(), now_ms);
}

}  // namespace link
}  // namespace core
