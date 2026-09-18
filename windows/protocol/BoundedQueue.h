#pragma once
#include "Protocol.h"
#include <deque>
namespace SweetDisplay::Transport {
// Caller owns synchronization; reject newest, never overwrite or grow unbounded.
template<class T>class BoundedQueue {
 std::deque<T> values;size_t bytes=0;
public:
 static constexpr size_t Capacity=3,ByteLimit=3*Protocol::MaxPayload;
 size_t peak=0,peakBytes=0;
 bool Push(T value){if(values.size()>=Capacity||value.payload.size()>ByteLimit-bytes)return false;bytes+=value.payload.size();values.push_back(std::move(value));peak=std::max(peak,values.size());peakBytes=std::max(peakBytes,bytes);return true;}
 T Pop(){Protocol::Require(!values.empty(),"empty queue");T x=std::move(values.front());values.pop_front();bytes-=x.payload.size();return x;}
 size_t Size()const{return values.size();}size_t Bytes()const{return bytes;}
};
}
