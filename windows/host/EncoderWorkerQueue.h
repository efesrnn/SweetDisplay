// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include <array>
#include <cstddef>
#include <utility>

namespace SweetDisplay::Encoding {
// Synchronization is owned by the caller. Reject-newest keeps latency bounded.
template<class T,size_t N>class EncoderWorkerQueue {
 static_assert(N>0,"encoder worker queue must be bounded");
 std::array<T,N> values{};size_t head=0,count=0,peak=0;
public:
 static constexpr size_t Capacity=N;
 bool Push(T value){if(count==N)return false;values[(head+count)%N]=std::move(value);++count;if(count>peak)peak=count;return true;}
 T Pop(){if(!count)throw "empty encoder worker queue";T value=std::move(values[head]);values[head]=T{};head=(head+1)%N;--count;return value;}
 void Clear(){while(count)(void)Pop();}
 size_t Size()const{return count;}
 size_t Peak()const{return peak;}
 bool Empty()const{return count==0;}
};

enum class EncoderWorkerPhase {Starting,Running,Finishing,Draining,Stopped,Failed};
inline bool ValidWorkerTransition(EncoderWorkerPhase from,EncoderWorkerPhase to){
 if(to==EncoderWorkerPhase::Failed)return from!=EncoderWorkerPhase::Stopped&&from!=EncoderWorkerPhase::Failed;
 switch(from){
  case EncoderWorkerPhase::Starting:return to==EncoderWorkerPhase::Running;
  case EncoderWorkerPhase::Running:return to==EncoderWorkerPhase::Finishing;
  case EncoderWorkerPhase::Finishing:return to==EncoderWorkerPhase::Draining;
  case EncoderWorkerPhase::Draining:return to==EncoderWorkerPhase::Stopped;
  default:return false;
 }
}
}
