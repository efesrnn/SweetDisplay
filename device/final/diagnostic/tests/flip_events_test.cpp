#include "../flip_events.h"
#include <array>

using sweetdisplay::FindFlipCompletion;
constexpr uint64_t kCookie = 0x1122334455667788ULL;
constexpr std::array<uint8_t, 32> Event(uint32_t type = 2,
                                       uint32_t length = 32,
                                       uint64_t cookie = kCookie) {
  std::array<uint8_t, 32> bytes{};
  for (int i = 0; i < 4; ++i) {
    bytes[i] = static_cast<uint8_t>(type >> (8 * i));
    bytes[i + 4] = static_cast<uint8_t>(length >> (8 * i));
  }
  for (int i = 0; i < 8; ++i)
    bytes[i + 8] = static_cast<uint8_t>(cookie >> (8 * i));
  return bytes;
}
constexpr auto kValid = Event();
static_assert(FindFlipCompletion(nullptr, 0, kCookie) == 0);
static_assert(FindFlipCompletion(kValid.data(), 32, kCookie) == 1);
static_assert(FindFlipCompletion(kValid.data(), 32, kCookie + 1) == 0);
static_assert(FindFlipCompletion(kValid.data(), 7, kCookie) == -1);
static_assert(FindFlipCompletion(kValid.data(), 31, kCookie) == -1);
static_assert(FindFlipCompletion(Event(2, 0).data(), 32, kCookie) == -1);
static_assert(FindFlipCompletion(Event(2, 7).data(), 32, kCookie) == -1);
static_assert(FindFlipCompletion(Event(2, 16).data(), 16, kCookie) == -1);
static_assert(FindFlipCompletion(Event(2, 33).data(), 32, kCookie) == -1);
static_assert(FindFlipCompletion(Event(1).data(), 32, kCookie) == 0);
static_assert(FindFlipCompletion(Event(99, 8).data(), 8, kCookie) == 0);
static_assert(FindFlipCompletion(Event(2, 32, 0).data(), 32, 0) == 1);

constexpr auto kMultiple = [] {
  std::array<uint8_t, 64> bytes{};
  const auto stale = Event(2, 32, kCookie - 1);
  for (int i = 0; i < 32; ++i) {
    bytes[i] = stale[i]; bytes[i + 32] = kValid[i];
  }
  return bytes;
}();
static_assert(FindFlipCompletion(kMultiple.data(), 64, kCookie) == 1);
static_assert(FindFlipCompletion(kMultiple.data(), 64, kCookie + 1) == 0);
static_assert(FindFlipCompletion(kMultiple.data(), 63, kCookie) == -1);

constexpr auto kUnaligned = [] {
  std::array<uint8_t, 33> bytes{};
  for (int i = 0; i < 32; ++i) bytes[i + 1] = kValid[i];
  return bytes;
}();
static_assert(FindFlipCompletion(kUnaligned.data() + 1, 32, kCookie) == 1);

constexpr auto kBadTail = [] {
  std::array<uint8_t, 40> bytes{};
  for (int i = 0; i < 32; ++i) bytes[i] = kValid[i];
  return bytes;
}();
static_assert(FindFlipCompletion(kBadTail.data(), 40, kCookie) == -1);

constexpr auto kUnknownThenValid = [] {
  std::array<uint8_t, 40> bytes{};
  bytes[0] = 99; bytes[4] = 8;
  for (int i = 0; i < 32; ++i) bytes[i + 8] = kValid[i];
  return bytes;
}();
static_assert(FindFlipCompletion(kUnknownThenValid.data(), 40, kCookie) == 1);
