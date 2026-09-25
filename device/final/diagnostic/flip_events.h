#pragma once
#include <stddef.h>
#include <stdint.h>

namespace sweetdisplay {
// Linux DRM event wire layout: header(type,length), then u64 user_data.
// Parse bytes, not cast pointers: read() may return multiple unaligned events.
constexpr uint32_t ReadLe32(const uint8_t* p) {
  return uint32_t(p[0]) | (uint32_t(p[1]) << 8) |
         (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}
constexpr uint64_t ReadLe64(const uint8_t* p) {
  return ReadLe32(p) | (uint64_t(ReadLe32(p + 4)) << 32);
}
// -1: malformed (fail closed), 0: no matching completion, 1: completion.
constexpr int FindFlipCompletion(const uint8_t* bytes, size_t size,
                                 uint64_t expected_cookie) {
  bool matched = false;
  size_t offset = 0;
  while (offset < size) {
    if (size - offset < 8) return -1;
    const uint32_t type = ReadLe32(bytes + offset);
    const uint32_t length = ReadLe32(bytes + offset + 4);
    if (length < 8 || length > size - offset) return -1;
    if (type == 2) {  // DRM_EVENT_FLIP_COMPLETE
      if (length < 32) return -1;
      if (ReadLe64(bytes + offset + 8) == expected_cookie) matched = true;
    }
    offset += length;
  }
  return matched ? 1 : 0;
}
}  // namespace sweetdisplay
