// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include <cstdint>
#include <cstring>
#include <cwchar>
#include <algorithm>
#include <windows.h>
#include <winioctl.h>

namespace SweetDisplay::Handoff {
inline constexpr GUID InterfaceId = {0x6e56a192,0xad96,0x482f,{0x8c,0x97,0x48,0x09,0x74,0xa9,0x45,0x3d}};
constexpr uint32_t Magic=0x53444648, Version=1, Capacity=3, NameChars=96;
constexpr ULONG StateIo=CTL_CODE(FILE_DEVICE_UNKNOWN,0x800,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA);
constexpr ULONG ConnectIo=CTL_CODE(FILE_DEVICE_UNKNOWN,0x801,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA);
constexpr ULONG FetchIo=CTL_CODE(FILE_DEVICE_UNKNOWN,0x802,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA);
constexpr ULONG AckIo=CTL_CODE(FILE_DEVICE_UNKNOWN,0x803,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA);
constexpr ULONG DisconnectIo=CTL_CODE(FILE_DEVICE_UNKNOWN,0x804,METHOD_BUFFERED,FILE_READ_DATA|FILE_WRITE_DATA);
#pragma pack(push,8)
struct Header { uint32_t magic=Magic, version=Version, size=0, reserved=0; };
struct Frame {
    uint64_t id=0, qpc=0, epoch=0, presentation=0;
    uint32_t width=0,height=0,format=0,flags=0,slot=0,reserved=0;
};
struct Stats {
    uint64_t source=0,delivered=0,producerDrops=0,hostQueueDrops=0,invalid=0,busy=0;
    uint32_t depth=0,held=0,highWater=0,reserved=0;
};
struct State {
    Header header{};
    uint64_t epoch=0,frequency=0,totalSource=0,noHost=0,contention=0;
    LUID adapter{};
    uint32_t width=0,height=0,format=0,active=0,connected=0;
    int32_t error=0;
    Stats stats{};
    Frame frame{};
};
struct Connect {
    Header header{};
    uint64_t epoch=0;
    LUID adapter{};
    uint32_t width=0,height=0,format=0,count=Capacity;
    wchar_t names[Capacity][NameChars]{};
};
struct Ack { Header header{}; uint64_t epoch=0,id=0; uint32_t slot=0,reserved=0; };
#pragma pack(pop)
static_assert(sizeof(Header)==16 && sizeof(Frame)==56 && sizeof(Ack)==40);
static_assert(sizeof(Connect)==624 && sizeof(State)==208);
template<class T> T Packet() { T p{}; p.header.size=sizeof(T); return p; }
template<class T> bool ValidPacket(const T& p,size_t length) {
    return length==sizeof(T) && p.header.magic==Magic && p.header.version==Version &&
        p.header.size==sizeof(T) && p.header.reserved==0;
}
inline bool ValidConnect(const Connect& p,size_t length) {
    if(!ValidPacket(p,length)||!p.epoch||p.count!=Capacity||p.width!=2400||p.height!=1080||p.format!=87) return false;
    for(uint32_t i=0;i<Capacity;++i) {
        const auto* n=p.names[i];
        if(wcsnlen_s(n,NameChars)>=NameChars || wcsncmp(n,L"Global\\SweetDisplay.",20)!=0 || wcslen(n)<30) return false;
        for(size_t j=20;n[j];++j) if(!((n[j]>=L'a'&&n[j]<=L'f')||(n[j]>=L'0'&&n[j]<=L'9')||n[j]==L'-'||n[j]==L'.')) return false;
        for(uint32_t j=0;j<i;++j) if(wcscmp(n,p.names[j])==0) return false;
    }
    return true;
}
inline bool ValidFrame(const Frame& f,uint64_t epoch,uint64_t lastId,uint64_t lastQpc,uint64_t now) {
    return f.epoch==epoch&&f.id>lastId&&f.qpc>lastQpc&&f.qpc<=now&&f.width==2400&&f.height==1080&&
        f.format==87&&f.slot<Capacity&&f.flags==0&&f.reserved==0;
}
inline double Milliseconds(uint64_t delta,uint64_t frequency) { return frequency ? double(delta)*1000.0/double(frequency) : 0.0; }
enum class SlotState { Free,Ready,Held };
struct Slot { SlotState state=SlotState::Free; uint64_t key=0; Frame frame{}; };
// Callers synchronize externally. Metadata and texture ownership are separate:
// dropping metadata leaves key 1 for the next producer to reclaim.
struct Queue {
    Slot slots[Capacity]{}; Stats stats{};
    int ProducerSlot() const {
        for(int i=0;i<int(Capacity);++i) if(slots[i].state==SlotState::Free) return i;
        int oldest=-1;
        for(int i=0;i<int(Capacity);++i) if(slots[i].state==SlotState::Ready && (oldest<0||slots[i].frame.id<slots[oldest].frame.id)) oldest=i;
        return oldest;
    }
    void Publish(int i,Frame frame) {
        if(slots[i].state==SlotState::Ready) ++stats.producerDrops;
        slots[i].state=SlotState::Ready; slots[i].key=1; frame.slot=uint32_t(i); slots[i].frame=frame; Recount();
    }
    int Fetch() {
        // The v1 consumer has exactly one outstanding lease.
        for(const auto& s:slots) if(s.state==SlotState::Held) return -1;
        int newest=-1;
        for(int i=0;i<int(Capacity);++i) if(slots[i].state==SlotState::Ready&&(newest<0||slots[i].frame.id>slots[newest].frame.id)) newest=i;
        if(newest<0) return -1;
        for(int i=0;i<int(Capacity);++i) if(i!=newest&&slots[i].state==SlotState::Ready) {slots[i].state=SlotState::Free;++stats.hostQueueDrops;}
        slots[newest].state=SlotState::Held; Recount(); return newest;
    }
    bool Acknowledge(const Ack& ack) {
        if(ack.slot>=Capacity) return false;
        auto& s=slots[ack.slot];
        if(s.state!=SlotState::Held||s.frame.id!=ack.id||s.frame.epoch!=ack.epoch) return false;
        s.state=SlotState::Free;s.key=0;++stats.delivered;Recount();return true;
    }
    void Recount() {
        stats.depth=stats.held=0;
        for(const auto& s:slots){stats.depth+=s.state==SlotState::Ready;stats.held+=s.state==SlotState::Held;}
        stats.highWater=(std::max)(stats.highWater,stats.depth+stats.held);
    }
};
}

