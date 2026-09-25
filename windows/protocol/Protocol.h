// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include <array>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <functional>
#include <limits>
namespace SweetDisplay::Protocol {
constexpr uint16_t Major=1, Minor=0, TouchProfileMinor=1, HeaderBytes=48;
constexpr uint32_t MaxAu=4*1024*1024, FrameBytes=64, MaxPayload=MaxAu+FrameBytes;
constexpr uint32_t FeatureVideo=1,FeatureTouch=2;
constexpr uint64_t TouchDescriptor=1ULL|(2ULL<<8)|(1ULL<<16)|(1ULL<<17)|(1ULL<<18)|(1ULL<<19);
enum class Type:uint16_t {Hello=1,Capabilities=2,Frame=3,Touch=4,Control=5,Telemetry=6,Heartbeat=7,CameraControl=8};
enum class Role:uint32_t {Host=1,Device=2};
struct Violation:std::runtime_error{using std::runtime_error::runtime_error;};
inline void Require(bool ok,const char* why){if(!ok)throw Violation(why);}
inline uint16_t U16(const uint8_t* p){return uint16_t(p[0])|(uint16_t(p[1])<<8);}
inline uint32_t U32(const uint8_t* p){return uint32_t(U16(p))|(uint32_t(U16(p+2))<<16);}
inline uint64_t U64(const uint8_t* p){return uint64_t(U32(p))|(uint64_t(U32(p+4))<<32);}
inline void Put16(uint8_t* p,uint16_t x){p[0]=uint8_t(x);p[1]=uint8_t(x>>8);}
inline void Put32(uint8_t* p,uint32_t x){Put16(p,uint16_t(x));Put16(p+2,uint16_t(x>>16));}
inline void Put64(uint8_t* p,uint64_t x){Put32(p,uint32_t(x));Put32(p+4,uint32_t(x>>32));}
inline uint64_t Nanoseconds(uint64_t ticks,uint64_t frequency){
 Require(frequency && frequency<=1000000000ULL,"clock frequency");
 Require(ticks/frequency<=UINT64_MAX/1000000000ULL,"clock overflow");
 auto whole=(ticks/frequency)*1000000000ULL, part=(ticks%frequency)*1000000000ULL/frequency;
 Require(whole<=UINT64_MAX-part,"clock addition overflow");return whole+part;
}
inline uint32_t Crc32(const uint8_t* p,size_t n){
 static const auto table=[] {std::array<uint32_t,256> t{};for(uint32_t i=0;i<256;++i){uint32_t x=i;for(int k=0;k<8;++k)x=(x>>1)^(0xEDB88320u & (0u-(x&1)));t[i]=x;}return t;}();
 uint32_t c=~0u;for(size_t i=0;i<n;++i)c=table[(c^p[i])&255]^(c>>8);return ~c;
}
struct Header {Type type=Type::Hello;uint32_t payload=0;uint64_t session=0,sequence=0,timestamp=0;};
inline std::array<uint8_t,HeaderBytes> SerializeHeader(const Header& h){
 std::array<uint8_t,HeaderBytes> b{};std::memcpy(b.data(),"SWDP",4);Put16(b.data()+4,Major);Put16(b.data()+6,Minor);
 Put16(b.data()+8,uint16_t(h.type));Put16(b.data()+10,HeaderBytes);Put32(b.data()+12,h.payload);
 Put64(b.data()+24,h.session);Put64(b.data()+32,h.sequence);Put64(b.data()+40,h.timestamp);return b;
}
inline Header ParseHeader(const uint8_t* b){
 Require(std::memcmp(b,"SWDP",4)==0,"magic");Require(U16(b+4)==Major,"major");Require(U16(b+6)==Minor,"minor");
 Require(U16(b+10)==HeaderBytes,"header size");Require(!U32(b+16)&&!U32(b+20),"header flags/reserved");
 Header h{Type(U16(b+8)),U32(b+12),U64(b+24),U64(b+32),U64(b+40)};
 Require(h.session&&h.sequence&&h.timestamp,"zero identity/time");Require(h.payload<=MaxPayload,"payload limit");
 switch(h.type){
 case Type::Hello:Require(h.payload==24,"hello size");break;
 case Type::Capabilities:case Type::Telemetry:case Type::Touch:Require(h.payload==32,"fixed payload size");break;
 case Type::Frame:Require(h.payload>FrameBytes && h.payload<=MaxPayload,"frame payload size");break;
 case Type::Control:case Type::Heartbeat:case Type::CameraControl:Require(h.payload==8,"control payload size");break;
 default:throw Violation("unknown message type");}
 return h;
}
struct FrameInfo {uint64_t id=0,sourceNs=0,pts=0;uint32_t codec=1,width=0,height=0,bytes=0,flags=0,crc=0;uint64_t sourceQpc=0,frequency=0;};
inline std::array<uint8_t,FrameBytes> SerializeFrameInfo(const FrameInfo& f){
 std::array<uint8_t,FrameBytes> b{};Put64(b.data(),f.id);Put64(b.data()+8,f.sourceNs);Put64(b.data()+16,f.pts);
 Put32(b.data()+24,f.codec);Put32(b.data()+28,f.width);Put32(b.data()+32,f.height);Put32(b.data()+36,f.bytes);Put32(b.data()+40,f.flags);Put32(b.data()+44,f.crc);Put64(b.data()+48,f.sourceQpc);Put64(b.data()+56,f.frequency);return b;
}
inline FrameInfo ParseFrameInfo(const uint8_t* b,uint32_t payload){
 FrameInfo f{U64(b),U64(b+8),U64(b+16),U32(b+24),U32(b+28),U32(b+32),U32(b+36),U32(b+40),U32(b+44),U64(b+48),U64(b+56)};
 Require(f.id&&f.sourceNs&&f.sourceQpc,"frame identity");Require(f.codec==1,"codec");
 Require(f.width&&f.height&&f.width<=4096&&f.height<=2160&&!(f.width&1)&&!(f.height&1),"geometry");
 Require(f.bytes && f.bytes<=MaxAu && payload==FrameBytes+f.bytes,"AU length");Require(!(f.flags&~15u),"frame flags");
 Require(f.sourceNs==Nanoseconds(f.sourceQpc,f.frequency),"source clock association");Require(f.pts<=INT64_MAX,"PTS range");return f;
}
// Only Annex B framing/NAL categories are checked here; decoding belongs to 3D.
inline uint32_t NalFlags(const uint8_t* b,size_t n){
 uint32_t flags=0;bool vcl=false;size_t starts=0;
 for(size_t i=0;i+3<n;++i){size_t offset=0;if(b[i]==0&&b[i+1]==0&&b[i+2]==1)offset=3;else if(i+4<n&&b[i]==0&&b[i+1]==0&&b[i+2]==0&&b[i+3]==1)offset=4;
  if(offset){if(!starts)Require(i==0,"Annex B prefix");++starts;Require(!(b[i+offset]&128),"NAL forbidden bit");auto t=b[i+offset]&31;Require(t>0&&t<24,"NAL type");vcl|=t==1||t==5;flags|=t==5?1u:t==7?2u:t==8?4u:0u;i+=offset-1;}}
 Require(vcl&&starts,"missing VCL");return flags;
}
struct Message {Header header;std::vector<uint8_t> payload;};
class Parser {
 std::array<uint8_t,HeaderBytes> header{};std::array<uint8_t,FrameBytes> meta{};
 size_t headUsed=0,metaUsed=0,used=0;Message current;bool poisoned=false,allocated=false;
 void Reset(){headUsed=metaUsed=used=0;allocated=false;current=Message{};}
public:
 size_t PeakPayload=0;
 size_t Needed()const {if(headUsed<HeaderBytes)return HeaderBytes-headUsed;if(current.header.type==Type::Frame&&!allocated)return FrameBytes-metaUsed;return current.payload.size()-used;}
 bool Partial()const{return headUsed!=0;}
 template<class F>void Feed(const uint8_t* p,size_t n,F&& receive){
  Require(!poisoned,"parser poisoned");try{while(n){
   if(headUsed<HeaderBytes){auto take=std::min(n,size_t(HeaderBytes)-headUsed);std::memcpy(header.data()+headUsed,p,take);headUsed+=take;p+=take;n-=take;if(headUsed<HeaderBytes)continue;current.header=ParseHeader(header.data());}
   if(current.header.type==Type::Frame&&!allocated){auto take=std::min(n,size_t(FrameBytes)-metaUsed);std::memcpy(meta.data()+metaUsed,p,take);metaUsed+=take;p+=take;n-=take;if(metaUsed<FrameBytes)continue;ParseFrameInfo(meta.data(),current.header.payload);}
   if(!allocated){current.payload.resize(current.header.payload);allocated=true;PeakPayload=std::max(PeakPayload,current.payload.size());if(current.header.type==Type::Frame){std::memcpy(current.payload.data(),meta.data(),FrameBytes);used=FrameBytes;}}
   auto take=std::min(n,current.payload.size()-used);std::memcpy(current.payload.data()+used,p,take);used+=take;p+=take;n-=take;
   if(used==current.payload.size()){receive(current);Reset();}
  }}catch(...){poisoned=true;throw;}
 }
 void End(){Require(!poisoned,"parser poisoned");if(headUsed){poisoned=true;throw Violation("truncated message");}}
};
inline std::vector<uint8_t> Wire(const Message& m){Require(m.payload.size()==m.header.payload,"serialize size");auto h=SerializeHeader(m.header);ParseHeader(h.data());std::vector<uint8_t> out(h.begin(),h.end());out.insert(out.end(),m.payload.begin(),m.payload.end());return out;}
struct Caps {uint32_t codecs=1,maxAu=MaxAu,width=4096,height=2160,features=FeatureVideo,clock=1;uint64_t extension=0;};
inline std::vector<uint8_t> Hello(Role role,uint16_t minimum=0,uint16_t maximum=0,uint32_t clock=1){std::vector<uint8_t> b(24);Put32(b.data(),uint32_t(role));Put16(b.data()+4,minimum);Put16(b.data()+6,maximum);Put32(b.data()+8,clock);Put64(b.data()+16,1);return b;}
inline std::vector<uint8_t> Capabilities(Caps c={}){std::vector<uint8_t>b(32);Put32(b.data(),c.codecs);Put32(b.data()+4,c.maxAu);Put32(b.data()+8,c.width);Put32(b.data()+12,c.height);Put32(b.data()+16,c.features);Put32(b.data()+20,c.clock);Put64(b.data()+24,c.extension);return b;}
enum class State {Hello,Capabilities,Ready,Closed};
class Connection {
 Role role;State state=State::Hello;uint64_t session=0,rx=0,tx=0,lastTimestamp=0,lastFrame=0,lastSource=0,lastPts=0;uint32_t peerClock=0;uint16_t maximumMinor=0,negotiatedMinor=0;Caps local{};bool helloSent=false,capsSent=false,hasFrame=false;
public:
 Caps peer;uint64_t frames=0,frameBytes=0,messages=0,wireBytes=0;uint64_t lastFrameSequence=0;uint64_t sequenceGaps=0,duplicates=0,outOfOrder=0;
 explicit Connection(Role r,uint64_t id=0,uint16_t maximum=0,Caps capabilities={}):role(r),session(id),maximumMinor(maximum),local(capabilities){Require(maximum<=TouchProfileMinor,"local minor");Require((local.features&FeatureVideo)&&!(local.features&~(FeatureVideo|FeatureTouch)),"local features");Require((local.features&FeatureTouch)?maximum>=TouchProfileMinor&&local.extension==TouchDescriptor:local.extension==0,"local extension");}
 State GetState()const{return state;}uint64_t Session()const{return session;}
 uint16_t NegotiatedMinor()const{return negotiatedMinor;}
 bool TouchNegotiated()const{return negotiatedMinor>=TouchProfileMinor&&(local.features&FeatureTouch)&&(peer.features&FeatureTouch)&&local.extension==TouchDescriptor&&peer.extension==TouchDescriptor;}
 std::vector<uint8_t> LocalHello()const{return Hello(role,0,maximumMinor,local.clock);}
 std::vector<uint8_t> LocalCapabilities()const{auto c=local;if(negotiatedMinor<TouchProfileMinor){c.features=FeatureVideo;c.extension=0;}return Capabilities(c);}
 Message Make(Type type,std::vector<uint8_t> payload,uint64_t now){
  Require(state!=State::Closed && tx!=UINT64_MAX,"closed/sequence overflow");
  if(type==Type::Hello){Require(!helloSent&&tx==0,"HELLO send order");helloSent=true;}
  else if(type==Type::Capabilities){Require(helloSent&&!capsSent&&state==State::Capabilities,"CAPABILITIES send order");capsSent=true;}
  else {Require(state==State::Ready,"send before ready");if(type==Type::Touch)Require(TouchNegotiated(),"unnegotiated TOUCH send");}
  Message m{{type,uint32_t(payload.size()),session,++tx,now},std::move(payload)};return m;
 }
 void Receive(const Message& m){
  try{
   const auto& h=m.header;Require(state!=State::Closed,"closed session");Require(m.payload.size()==h.payload,"semantic payload size");
   auto serialized=SerializeHeader(h);ParseHeader(serialized.data());
   if(h.sequence!=rx+1){if(h.sequence==rx)++duplicates;else if(h.sequence<rx)++outOfOrder;else ++sequenceGaps;throw Violation("sequence integrity");}
   if(!session){Require(role==Role::Device&&state==State::Hello,"missing session");session=h.session;}
   Require(h.session==session,"session mismatch");Require(h.timestamp>=lastTimestamp,"timestamp regression");
   auto p=m.payload.data();
   if(state==State::Hello){Require(h.type==Type::Hello,"HELLO required");Require(U32(p)==uint32_t(role==Role::Host?Role::Device:Role::Host),"peer role");auto peerMin=U16(p+4),peerMax=U16(p+6);Require(peerMin<=peerMax&&peerMin<=maximumMinor,"minor negotiation");negotiatedMinor=(std::min)(maximumMinor,peerMax);Require(negotiatedMinor>=peerMin,"minor overlap");Require(U32(p+8)<=1&&!U32(p+12)&&U64(p+16)==1,"HELLO clock/features");peerClock=U32(p+8);state=State::Capabilities;}
   else if(state==State::Capabilities){Require(h.type==Type::Capabilities&&helloSent&&capsSent,"CAPABILITIES required");peer={U32(p),U32(p+4),U32(p+8),U32(p+12),U32(p+16),U32(p+20),U64(p+24)};Require((peer.codecs&1)&&!(peer.codecs&~1u)&&peer.maxAu>0&&peer.maxAu<=MaxAu&&peer.width>0&&peer.width<=4096&&peer.height>0&&peer.height<=2160&&peer.clock==peerClock,"capability mismatch");if(negotiatedMinor<TouchProfileMinor)Require(peer.features==FeatureVideo&&!peer.extension,"legacy capability profile");else Require((peer.features&FeatureVideo)&&!(peer.features&~(FeatureVideo|FeatureTouch))&&((peer.features&FeatureTouch)?peer.extension==TouchDescriptor:peer.extension==0),"extension capability profile");state=State::Ready;}
   else if(h.type==Type::Frame){Require(role==Role::Device,"unexpected inbound FRAME");auto f=ParseFrameInfo(p,h.payload);Require(f.width<=peer.width&&f.height<=peer.height&&f.bytes<=peer.maxAu,"negotiated frame limits");Require((f.flags&7)==NalFlags(p+FrameBytes,f.bytes),"NAL/metadata association");Require(Crc32(p+FrameBytes,f.bytes)==f.crc,"AU CRC");Require(hasFrame||(f.flags&7)==7,"session needs SPS/PPS/IDR");Require(f.id>lastFrame&&f.sourceNs>lastSource&&(!hasFrame||f.pts>lastPts),"frame/source/PTS ordering");lastFrame=f.id;lastSource=f.sourceNs;lastPts=f.pts;hasFrame=true;++frames;frameBytes+=f.bytes;lastFrameSequence=h.sequence;}
   else if(h.type==Type::Heartbeat){/* bounded opaque echo token */}
   else if(h.type==Type::Telemetry){Require(role==Role::Host,"unexpected telemetry");}
   else if(h.type==Type::Touch){Require(TouchNegotiated(),"unnegotiated TOUCH");}
   else if(h.type==Type::Control){Require(U32(p)==1||U32(p)==2,"control operation");Require(!U32(p+4),"control reserved");}
   else throw Violation("unnegotiated message");
   rx=h.sequence;lastTimestamp=h.timestamp;++messages;wireBytes+=HeaderBytes+h.payload;
  }catch(...){state=State::Closed;throw;}
 }
};
} // namespace
