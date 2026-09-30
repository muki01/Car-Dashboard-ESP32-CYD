/**
 * @file persist.h
 * Versioned, CRC protected blobs on top of core::Storage.
 *
 * Layout: [magic u32][version u16][payload size u16][crc32 u32][payload]
 *
 * Structs stored this way must be trivially copyable and are extended by
 * APPENDING fields only. A blob written by an older firmware (shorter payload)
 * is loaded as a prefix; new fields keep the caller's default values.
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#include "storage.h"

namespace core::persist {

enum class LoadResult : uint8_t {
  Ok,        ///< exact match
  Upgraded,  ///< older/newer layout, common prefix loaded
  Missing,   ///< nothing stored under the key
  Corrupt,   ///< bad magic or CRC, caller keeps defaults
};

LoadResult load(Storage& storage, const char* key, void* data, size_t size);
bool save(Storage& storage, const char* key, uint16_t version, const void* data, size_t size);

}  // namespace core::persist
