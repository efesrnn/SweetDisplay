// Diagnostic test producer ledger only. Not part of the driver/Host frame ABI.
#pragma once
#include <windows.h>
#include <cstdint>
#include <string>
#include <stdexcept>
namespace SweetDisplay::PatternOracle {
constexpr uint32_t Magic=0x53444f52,Version=1,Capacity=4096;
struct alignas(8) Entry {volatile LONG counter;LONG reserved;uint64_t qpc;};
struct alignas(8) Ledger {
 uint32_t magic,version,nonce,pid;uint64_t hwnd,frequency,generation;
 volatile LONG published,hostCounter;Entry entries[Capacity];
};
inline std::wstring Name(uint32_t nonce){wchar_t s[80]{};swprintf_s(s,L"Local\\SweetDisplay.PatternOracle.%08X",nonce);return s;}
inline uint32_t Load(volatile LONG* p){return uint32_t(InterlockedCompareExchange(p,0,0));}
enum Failure:uint32_t {Zero=1,Nonce=2,Regression=4,Unpublished=8,Expired=16,Future=32};
inline uint32_t Evaluate(uint32_t expected,uint32_t observed,uint32_t previous,uint32_t counter,uint32_t published,bool found,uint64_t renderQpc,uint64_t sourceQpc){
 uint32_t f=(!observed||!counter)?Zero:0;if(observed!=expected)f|=Nonce;
 if(f)return f; // Invalid content is still a failure; do not interpret its bits as a valid ledger key.
 if(counter<previous)f|=Regression;if(counter>published)f|=Unpublished;
 if(!found)f|=Expired;else if(renderQpc>sourceQpc)f|=Future;return f;
}
class Mapping {
 HANDLE handle=nullptr;
public:
 Ledger* data=nullptr;
 ~Mapping(){if(data)UnmapViewOfFile(data);if(handle)CloseHandle(handle);}
 Mapping()=default;Mapping(const Mapping&)=delete;Mapping& operator=(const Mapping&)=delete;
 void Open(uint32_t nonce,bool writer,HWND window=nullptr){
  const auto name=Name(nonce);
  if(writer){handle=CreateFileMappingW(INVALID_HANDLE_VALUE,nullptr,PAGE_READWRITE,0,sizeof(Ledger),name.c_str());if(handle&&GetLastError()==ERROR_ALREADY_EXISTS)throw std::runtime_error("Oracle mapping collision");}
  else handle=OpenFileMappingW(FILE_MAP_ALL_ACCESS,FALSE,name.c_str());
  if(!handle)throw std::runtime_error("Cannot open diagnostic producer ledger");
  data=static_cast<Ledger*>(MapViewOfFile(handle,FILE_MAP_ALL_ACCESS,0,0,sizeof(Ledger)));
  if(!data)throw std::runtime_error("Cannot map diagnostic producer ledger");
  if(writer){ZeroMemory(data,sizeof(*data));LARGE_INTEGER q{},f{};QueryPerformanceCounter(&q);QueryPerformanceFrequency(&f);data->version=Version;data->nonce=nonce;data->pid=GetCurrentProcessId();data->hwnd=uint64_t(window);data->frequency=f.QuadPart;data->generation=q.QuadPart;MemoryBarrier();data->magic=Magic;}
  if(data->magic!=Magic||data->version!=Version||data->nonce!=nonce)throw std::runtime_error("Diagnostic ledger identity mismatch");
 }
 void Publish(uint32_t counter,uint64_t qpc){auto& e=data->entries[counter%Capacity];InterlockedExchange(&e.counter,0);e.qpc=qpc;InterlockedExchange(&e.counter,LONG(counter));InterlockedExchange(&data->published,LONG(counter));}
 bool Lookup(uint32_t counter,uint64_t& qpc){auto& e=data->entries[counter%Capacity];for(int retry=0;retry<3;++retry){auto a=Load(&e.counter);auto stamp=e.qpc;MemoryBarrier();auto b=Load(&e.counter);if(a==counter&&b==counter){qpc=stamp;return true;}}return false;}
};
}
