// SweetDisplay diagnostic ETW payloads; not the streaming/phone protocol.
#pragma once
#include <windows.h>
#include <cstdint>
namespace SweetDisplay::Diagnostic {
inline constexpr GUID Provider = {0x2a35dbb1,0xc9c1,0x4c55,{0xa0,0x80,0x91,0x96,0x89,0x4b,0x84,0x03}};
inline constexpr uint64_t MetadataKeyword = 1, CaptureKeyword = 2;
inline constexpr uint32_t Magic = 0x31464453; // SDF1
inline constexpr uint16_t Version = 1;
inline constexpr uint32_t ChunkBytes = 48 * 1024, MaxBytes = 64 * 1024 * 1024;
enum : USHORT { FrameEvent=1, BeginEvent=2, ChunkEvent=3, EndEvent=4, ErrorEvent=5 };
#pragma pack(push, 1)
struct Frame {
    uint32_t magic=Magic;
    uint16_t version=Version, size=sizeof(Frame);
    uint64_t frameId=0, acquiredQpc=0, qpcFrequency=0, presentQpc=0, intervalQpc=0;
    uint32_t width=0, height=0, format=0, presentationFrameNumber=0;
    uint32_t adapterLow=0;
    int32_t adapterHigh=0;
};
struct Chunk {
    uint64_t frameId;
    uint32_t offset, bytes;
};
struct End {
    uint64_t frameId, hash;
    uint32_t bytes, rowBytes;
};
struct Error {
    uint64_t frameId;
    int32_t code;
    uint32_t stage;
};
#pragma pack(pop)
static_assert(sizeof(Frame)==72);
static_assert(sizeof(Chunk)==16);
inline uint64_t Hash(const unsigned char* p, size_t n, uint64_t hash=14695981039346656037ull) {
    for (size_t i=0;i<n;++i) { hash ^= p[i]; hash *= 1099511628211ull; }
    return hash;
}
}

