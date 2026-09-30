#include "nvs_storage.h"

#include "../core/log.h"

namespace hal {

bool NvsStorage::begin(const char* name_space) {
  open_ = prefs_.begin(name_space, false);
  if (!open_) LOG_E("NVS", "cannot open namespace '%s'", name_space);
  return open_;
}

size_t NvsStorage::size(const char* key) {
  if (!open_ || !prefs_.isKey(key)) return 0;
  return prefs_.getBytesLength(key);
}

bool NvsStorage::read(const char* key, void* data, size_t len) {
  if (size(key) != len) return false;
  return prefs_.getBytes(key, data, len) == len;
}

bool NvsStorage::write(const char* key, const void* data, size_t len) {
  return open_ && prefs_.putBytes(key, data, len) == len;
}

void NvsStorage::erase_all() {
  if (open_) prefs_.clear();
}

}  // namespace hal
