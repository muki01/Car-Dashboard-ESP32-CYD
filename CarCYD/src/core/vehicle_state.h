/**
 * @file vehicle_state.h
 * Snapshot of everything the firmware knows about the vehicle.
 *
 * Written by the active DataLink, read by the UI, the alert manager and the
 * trip computer. Everything runs in the main loop, so no locking is needed.
 */
#pragma once

#include <stdint.h>

#include "types.h"

namespace core {

/** A value is considered stale (shown as "--") when not refreshed for this long. */
constexpr uint32_t kSignalStaleMs = 2500;

struct SignalValue {
  float value = 0.0f;
  uint32_t updated_ms = 0;
  bool valid = false;
};

// ---- Diagnostic trouble codes ----------------------------------------------

enum class DtcStatus : uint8_t { Stored = 0, Pending, Permanent };

/** SAE J2012 code: bits 15..14 system (P/C/B/U), 13..0 digits, e.g. 0x0301 = P0301. */
struct Dtc {
  uint16_t code;
  DtcStatus status;
};

enum class DtcOperation : uint8_t { Idle = 0, Reading, Clearing };
enum class DtcResult : uint8_t { None = 0, ReadOk, ClearOk, Failed, NotConnected };

struct DtcState {
  static constexpr uint8_t kMaxCodes = 24;

  Dtc codes[kMaxCodes] = {};
  uint8_t count = 0;
  bool mil_on = false;
  bool mil_known = false;   ///< MIL status has been received at least once
  bool scanned = false;     ///< codes have been read at least once
  uint32_t last_scan_ms = 0;
  DtcOperation operation = DtcOperation::Idle;
  DtcResult last_result = DtcResult::None;
  uint32_t revision = 0;    ///< incremented on every change
};

// ---- Vehicle state ------------------------------------------------------------

class VehicleState {
 public:
  void set(SignalId id, float value, uint32_t now_ms);
  void invalidate(SignalId id);
  void invalidate_all();

  const SignalValue& get(SignalId id) const { return signals_[static_cast<uint8_t>(id)]; }

  /** True if the signal is valid and was refreshed recently. */
  bool fresh(SignalId id, uint32_t now_ms) const;

  /** Value if fresh, `fallback` otherwise. */
  float value_or(SignalId id, uint32_t now_ms, float fallback) const;

  bool engine_running(uint32_t now_ms) const;

  // Link status (written by the data link).
  LinkState link_state = LinkState::Disconnected;
  bool simulated = false;

  // Wall clock supplied by the vehicle interface (minutes since midnight).
  bool clock_valid = false;
  uint16_t clock_minutes = 0;

  DtcState dtc;

 private:
  SignalValue signals_[kSignalCount];
};

/** The single vehicle state instance. */
VehicleState& vehicle();

}  // namespace core
