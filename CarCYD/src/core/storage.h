/**
 * @file storage.h
 * Persistent key/value blob storage interface.
 *
 * Implemented by the platform (NVS on the ESP32, memory in the simulator).
 * Keys must be at most 15 characters (NVS limit).
 */
#pragma once

#include <stddef.h>

namespace core {

class Storage {
 public:
  virtual ~Storage() = default;

  /** Reads exactly `len` bytes. Returns false if the key is missing or has another size. */
  virtual bool read(const char* key, void* data, size_t len) = 0;

  /** Returns the stored size of `key`, or 0 if missing. */
  virtual size_t size(const char* key) = 0;

  virtual bool write(const char* key, const void* data, size_t len) = 0;
  virtual void erase_all() = 0;
};

/** Blob keys used by the firmware. */
namespace storage_key {
constexpr const char* kSettings = "settings";
constexpr const char* kTouchCal = "touch_cal";
constexpr const char* kTrip = "trip";
}  // namespace storage_key

}  // namespace core
