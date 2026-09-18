// Transport-neutral compressed-output callback; no GPU/codec ownership crosses it.
#pragma once
#include <cstdint>
namespace SweetDisplay::Protocol {
struct EncodedView {uint64_t id,sourceQpc,frequency,pts;uint32_t width,height,flags,bytes;const uint8_t* data;};
struct EncodedSink {virtual ~EncodedSink()=default;virtual void Consume(const EncodedView& view)=0;};
}
