#include "../SweetDisplayDriver/MonitorConfig.h"
#include <cassert>
#include <cstdio>
#include <cstring>

int main() {
    using namespace SweetDisplay;
    // Decode the descriptor independently of the constants used by callbacks.
    unsigned checksum = 0;
    for (auto value : Edid) checksum += value;
    assert((checksum & 255) == 0);
    const unsigned char header[] = {0,255,255,255,255,255,255,0};
    assert(std::memcmp(Edid, header, 8) == 0);
    assert(Edid[18] == 1 && Edid[19] == 4 && Edid[126] == 0);
    const unsigned maker = (Edid[8] << 8) | Edid[9];
    assert(((maker >> 10) & 31) == 19 && ((maker >> 5) & 31) == 23 && (maker & 31) == 20);
    assert(Edid[10] == 1 && Edid[11] == 0);
    assert(Edid[12] == 0 && Edid[13] == 0 && Edid[14] == 0 && Edid[15] == 0);
    assert(std::memcmp(Edid + 77, "SweetDisplay\n", 13) == 0);
    const auto* d = Edid + 54;
    const unsigned width = d[2] | ((d[4] & 0xf0) << 4);
    const unsigned height = d[5] | ((d[7] & 0xf0) << 4);
    const unsigned hblank = d[3] | ((d[4] & 0x0f) << 8);
    const unsigned vblank = d[6] | ((d[7] & 0x0f) << 8);
    const unsigned clock = (d[0] | (d[1] << 8)) * 10000;
    assert(width == 2400 && height == 1080);
    assert(hblank == HorizontalBlank && vblank == VerticalBlank);
    assert(clock == (width + hblank) * (height + vblank) * 60);
    assert(Modes[0].width == width && Modes[0].height == height && Modes[0].refresh == 60);
    assert(sizeof(Modes) / sizeof(Modes[0]) == ModeCount);
    for (const auto& mode : Modes) {
        assert(mode.width * 9 == mode.height * 20);
        assert(mode.refresh == 30 || mode.refresh == 60);
    }
    int buffer = 0;
    assert(CheckBuffer(0, nullptr) == BufferStatus::CountOnly);
    for (unsigned capacity = 1; capacity < ModeCount; ++capacity)
        assert(CheckBuffer(capacity, &buffer) == BufferStatus::TooSmall);
    assert(CheckBuffer(ModeCount, nullptr) == BufferStatus::Invalid);
    assert(CheckBuffer(ModeCount, &buffer) == BufferStatus::Fill);
    assert(CheckBuffer(ModeCount + 1, &buffer) == BufferStatus::Fill);
    std::puts("PASS: EDID checksum/identity/DTD, exact 60 Hz, aspect ratios and mode-buffer boundaries");
}
