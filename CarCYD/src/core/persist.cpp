#include "persist.h"

#include <string.h>

#include "log.h"
#include "util.h"

namespace core::persist {
namespace {

constexpr uint32_t kMagic = 0x44594343;  // "CCYD"
constexpr size_t kMaxBlob = 512;

struct Header {
  uint32_t magic;
  uint16_t version;
  uint16_t size;
  uint32_t crc;
};

}  // namespace

LoadResult load(Storage& storage, const char* key, void* data, size_t size) {
  const size_t stored = storage.size(key);
  if (stored == 0) return LoadResult::Missing;
  if (stored < sizeof(Header) || stored > kMaxBlob) {
    LOG_W("NVS", "'%s' has invalid size %u", key, static_cast<unsigned>(stored));
    return LoadResult::Corrupt;
  }

  uint8_t buffer[kMaxBlob];
  if (!storage.read(key, buffer, stored)) return LoadResult::Corrupt;

  Header header;
  memcpy(&header, buffer, sizeof(header));
  const uint8_t* payload = buffer + sizeof(Header);
  const size_t payload_size = stored - sizeof(Header);

  if (header.magic != kMagic || header.size != payload_size || header.crc != crc32(payload, payload_size)) {
    LOG_W("NVS", "'%s' failed integrity check", key);
    return LoadResult::Corrupt;
  }

  memcpy(data, payload, payload_size < size ? payload_size : size);
  return payload_size == size ? LoadResult::Ok : LoadResult::Upgraded;
}

bool save(Storage& storage, const char* key, uint16_t version, const void* data, size_t size) {
  if (size + sizeof(Header) > kMaxBlob) return false;

  uint8_t buffer[kMaxBlob];
  const Header header{kMagic, version, static_cast<uint16_t>(size), crc32(data, size)};
  memcpy(buffer, &header, sizeof(header));
  memcpy(buffer + sizeof(header), data, size);

  const bool ok = storage.write(key, buffer, sizeof(header) + size);
  if (!ok) LOG_E("NVS", "writing '%s' failed", key);
  return ok;
}

}  // namespace core::persist
