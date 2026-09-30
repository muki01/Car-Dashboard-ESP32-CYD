/**
 * @file data_link.h
 * Abstraction of the vehicle data source.
 *
 * A DataLink fills the VehicleState (signals, link state, DTCs) and executes
 * diagnostic requests asynchronously. The UI never talks to a transport
 * directly; it only reads VehicleState and calls core::link::request_*().
 *
 * Implementations:
 *   - SimLink  : demo mode, simulates a driving car (sim_link.h)
 *   - NullLink : no interface connected (below)
 *   - the vehicle interface ESP32 link will be added here (UART / ESP-NOW)
 */
#pragma once

#include <stdint.h>

#include "vehicle_state.h"

namespace core {

class DataLink {
 public:
  virtual ~DataLink() = default;

  virtual const char* name() const = 0;

  /** Called when the link becomes active. */
  virtual void begin(VehicleState& state, uint32_t now_ms) = 0;

  /** Called when another link is activated. */
  virtual void end(VehicleState& state) { (void)state; }

  /** Called from the main loop as often as possible. Must not block. */
  virtual void poll(VehicleState& state, uint32_t now_ms) = 0;

  /** Starts reading stored/pending/permanent DTCs. Returns false if rejected. */
  virtual bool request_dtc_read(VehicleState& state, uint32_t now_ms) = 0;

  /** Starts clearing the DTCs (OBD mode 04). Returns false if rejected. */
  virtual bool request_dtc_clear(VehicleState& state, uint32_t now_ms) = 0;
};

/** Placeholder used when no vehicle interface is available. */
class NullLink final : public DataLink {
 public:
  const char* name() const override { return "none"; }
  void begin(VehicleState& state, uint32_t now_ms) override;
  void poll(VehicleState& state, uint32_t now_ms) override;
  bool request_dtc_read(VehicleState& state, uint32_t now_ms) override;
  bool request_dtc_clear(VehicleState& state, uint32_t now_ms) override;
};

namespace link {

/** Activates `link` (ends the previous one). */
void set_active(DataLink* link, uint32_t now_ms);
DataLink* active();

void poll(uint32_t now_ms);
bool request_dtc_read(uint32_t now_ms);
bool request_dtc_clear(uint32_t now_ms);

}  // namespace link
}  // namespace core
