/**
 * @file nvs_storage.h
 * core::Storage implementation on the ESP32 NVS partition (Preferences).
 */
#pragma once

#include <Preferences.h>

#include "../core/storage.h"

namespace hal {

class NvsStorage final : public core::Storage {
 public:
  bool begin(const char* name_space);

  bool read(const char* key, void* data, size_t len) override;
  size_t size(const char* key) override;
  bool write(const char* key, const void* data, size_t len) override;
  void erase_all() override;

 private:
  Preferences prefs_;
  bool open_ = false;
};

}  // namespace hal
