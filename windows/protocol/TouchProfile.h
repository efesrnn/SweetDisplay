// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include "Protocol.h"
#include <array>

namespace SweetDisplay::Touch {
namespace P=SweetDisplay::Protocol;
constexpr uint16_t Profile=1;
constexpr uint32_t CoordinateMaximum=65535,MaximumContacts=2;
constexpr uint32_t ConfigPressure=1,ConfigContentNative=2,ConfigRotationAppliedOnce=4,ConfigDeviceElapsedRealtimeNs=8;
constexpr uint32_t RequiredConfigFlags=ConfigPressure|ConfigContentNative|ConfigRotationAppliedOnce|ConfigDeviceElapsedRealtimeNs;
constexpr uint16_t EventPressureValid=1,EventPrimary=2,EventRotationShift=8,EventRotationMask=3u<<EventRotationShift;
enum class Operation:uint16_t {Configure=1,Ready=2,Contact=3};
enum class Action:uint16_t {Down=1,Move=2,Up=3,Cancel=4};
struct Configuration {uint32_t coordinateMaximum=CoordinateMaximum,width=2400,height=1080,maxContacts=MaximumContacts,flags=RequiredConfigFlags;uint64_t targetToken=0;};
struct Event {uint16_t contact=0;Action action=Action::Down;uint32_t x=0,y=0;uint16_t pressure=0,flags=0;uint32_t activeMask=0;uint64_t deviceTimestampNs=0;};

inline std::vector<uint8_t> Configure(const Configuration& c,Operation operation=Operation::Configure){
 P::Require(operation==Operation::Configure||operation==Operation::Ready,"touch configuration operation");
 P::Require(c.coordinateMaximum==CoordinateMaximum&&c.width&&c.height&&c.width<=4096&&c.height<=2160&&c.maxContacts==MaximumContacts&&c.flags==RequiredConfigFlags&&c.targetToken,"touch configuration");
 std::vector<uint8_t>b(32);P::Put16(b.data(),Profile);P::Put16(b.data()+2,uint16_t(operation));P::Put32(b.data()+4,c.coordinateMaximum);P::Put32(b.data()+8,c.width);P::Put32(b.data()+12,c.height);P::Put32(b.data()+16,c.maxContacts);P::Put32(b.data()+20,c.flags);P::Put64(b.data()+24,c.targetToken);return b;
}
inline Configuration ParseConfiguration(const uint8_t* b,Operation operation){
 P::Require(P::U16(b)==Profile&&P::U16(b+2)==uint16_t(operation),"touch configuration profile/operation");
 Configuration c{P::U32(b+4),P::U32(b+8),P::U32(b+12),P::U32(b+16),P::U32(b+20),P::U64(b+24)};Configure(c,operation);return c;
}
inline std::vector<uint8_t> Contact(const Event& e){
 std::vector<uint8_t>b(32);P::Put16(b.data(),Profile);P::Put16(b.data()+2,uint16_t(Operation::Contact));P::Put16(b.data()+4,e.contact);P::Put16(b.data()+6,uint16_t(e.action));P::Put32(b.data()+8,e.x);P::Put32(b.data()+12,e.y);P::Put16(b.data()+16,e.pressure);P::Put16(b.data()+18,e.flags);P::Put32(b.data()+20,e.activeMask);P::Put64(b.data()+24,e.deviceTimestampNs);return b;
}
inline Event ParseEvent(const uint8_t* b){
 P::Require(P::U16(b)==Profile&&P::U16(b+2)==uint16_t(Operation::Contact),"touch contact profile/operation");
 return {P::U16(b+4),Action(P::U16(b+6)),P::U32(b+8),P::U32(b+12),P::U16(b+16),P::U16(b+18),P::U32(b+20),P::U64(b+24)};
}
class ContactState {
 Configuration configuration;uint32_t active=0;uint64_t lastTimestamp=0;
public:
 explicit ContactState(Configuration c):configuration(c){Configure(c);}
 uint32_t ActiveMask()const{return active;}
 void ReleaseAll(){active=0;lastTimestamp=0;}
 void Apply(const Event& e){
  P::Require(e.contact<configuration.maxContacts&&e.x<=configuration.coordinateMaximum&&e.y<=configuration.coordinateMaximum,"touch contact bounds");
  P::Require(uint16_t(e.action)>=uint16_t(Action::Down)&&uint16_t(e.action)<=uint16_t(Action::Cancel),"touch action");
  P::Require((e.flags&~uint16_t(EventPressureValid|EventPrimary|EventRotationMask))==0&&((e.flags&EventRotationMask)>>EventRotationShift)<=3,"touch event flags");
  P::Require((e.flags&EventPressureValid)?e.pressure<=1024:e.pressure==0,"touch pressure");
  P::Require(e.activeMask<(1u<<configuration.maxContacts)&&e.deviceTimestampNs&&e.deviceTimestampNs>=lastTimestamp,"touch active mask/time");
  const uint32_t bit=1u<<e.contact;const bool was=(active&bit)!=0;
  if(e.action==Action::Down){P::Require(!was&&(e.activeMask&bit),"duplicate active contact");}
  else if(e.action==Action::Move){P::Require(was&&(e.activeMask&bit),"MOVE for inactive contact");}
  else P::Require(was&&!(e.activeMask&bit),"UP/CANCEL for inactive contact");
  P::Require(e.activeMask==(e.action==Action::Down?active|bit:e.action==Action::Move?active:active&~bit),"active contact state");
  active=e.activeMask;lastTimestamp=e.deviceTimestampNs;
 }
};
class Endpoint {
public:
 virtual ~Endpoint()=default;
 virtual Configuration CurrentConfiguration()=0;
 virtual void Begin(uint64_t session,const Configuration& configuration)=0;
 virtual void Accept(uint64_t session,const Event& event)=0;
 virtual void Tick()=0;
 virtual void End() noexcept=0;
};
}
