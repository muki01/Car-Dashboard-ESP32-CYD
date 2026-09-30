#include "util.h"

namespace core {

uint32_t crc32(const void* data, size_t len) {
  const auto* p = static_cast<const uint8_t*>(data);
  uint32_t crc = 0xFFFFFFFFu;
  while (len--) {
    crc ^= *p++;
    for (int bit = 0; bit < 8; ++bit) {
      crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
    }
  }
  return ~crc;
}

}  // namespace core
