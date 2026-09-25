#include "../Protocol.h"
#include "../BoundedQueue.h"
#include "../ResyncGate.h"
#include "../TouchProfile.h"
#include "../MicroTiming.h"
#include "../../host/TransportContentPolicy.h"
#include <cstdio>
#include <random>
using namespace SweetDisplay::Protocol;
static uint64_t checks=0;
void Check(bool ok){++checks;if(!ok)throw std::runtime_error("test assertion");}
template<class F>void Reject(F fn){bool bad=false;try{fn();}catch(const Violation&){bad=true;}Check(bad);}
Message Example(){
 std::vector<uint8_t> au{0,0,0,1,0x67,1,0,0,1,0x68,2,0,0,1,0x65,3};
 FrameInfo f{12,1000000000ULL,0,1,2400,1080,uint32_t(au.size()),15,Crc32(au.data(),au.size()),10000000,10000000};
 auto b=SerializeFrameInfo(f);Message m{{Type::Frame,FrameBytes+uint32_t(au.size()),123,3,1000000001ULL},std::vector<uint8_t>(b.begin(),b.end())};m.payload.insert(m.payload.end(),au.begin(),au.end());return m;
}
Connection Ready(){Connection h(Role::Host,123),d(Role::Device);auto a=h.Make(Type::Hello,Hello(Role::Host),1);d.Receive(a);a=d.Make(Type::Hello,Hello(Role::Device),1);h.Receive(a);auto hc=h.Make(Type::Capabilities,Capabilities(),2);auto dc=d.Make(Type::Capabilities,Capabilities(),2);h.Receive(dc);d.Receive(hc);Check(h.GetState()==State::Ready&&d.GetState()==State::Ready);return d;}
int main(){try{
 Check(Crc32(reinterpret_cast<const uint8_t*>("123456789"),9)==0xCBF43926u);
 {using namespace SweetDisplay::MicroTiming;
  DegradationDetector::Decision decision{};
  DegradationDetector healthy;for(uint64_t second=0;second<=20;++second)Check(!healthy.Observe(second*1000,second*55,second*55,1000,decision));Check(!healthy.Fired());
  DegradationDetector slowSource;for(uint64_t second=0;second<=20;++second)Check(!slowSource.Observe(second*1000,second*30,second*30,1000,decision));Check(!slowSource.Fired());
  DegradationDetector transient;uint64_t transientHost=0;for(uint64_t second=0;second<=20;++second){if(second)transientHost+=second==5?0:55;Check(!transient.Observe(second*1000,second*55,transientHost,1000,decision));}Check(!transient.Fired());
  DegradationDetector degraded;uint32_t fires=0;for(uint64_t second=0;second<=20;++second)if(degraded.Observe(second*1000,second*55,second*27,1000,decision))++fires;Check(fires==1&&degraded.Fired());Check(decision.qpc==10000&&decision.seconds==10.0&&decision.sourceFps==55.0&&decision.hostFps==27.0&&decision.ratio<0.50);
  constexpr uint64_t trigger=FlightLayout::PreCapacity+12,postRecords=1000;std::vector<int64_t> ring(size_t(FlightLayout::PreCapacity+FlightLayout::PostCapacity),-1);
  for(uint64_t sequence=0;sequence<trigger;++sequence)ring[size_t(FlightLayout::Index(sequence,FlightLayout::NoTrigger))]=int64_t(sequence);
  for(uint64_t sequence=trigger;sequence<trigger+postRecords;++sequence)ring[size_t(FlightLayout::Index(sequence,trigger))]=int64_t(sequence);
  Check(FlightLayout::PreBegin(trigger)==12&&FlightLayout::PostEnd(trigger+FlightLayout::PostCapacity+5,trigger)==trigger+FlightLayout::PostCapacity);
  bool prePreserved=true;for(uint64_t sequence=12;sequence<trigger;++sequence)if(ring[size_t(FlightLayout::Index(sequence,FlightLayout::NoTrigger))]!=int64_t(sequence)){prePreserved=false;break;}Check(prePreserved);
  bool postPreserved=true;for(uint64_t sequence=trigger;sequence<trigger+postRecords;++sequence)if(ring[size_t(FlightLayout::Index(sequence,trigger))]!=int64_t(sequence)){postPreserved=false;break;}Check(postPreserved);
  Check(!FlightLayout::PostComplete(69999,10000,1000)&&FlightLayout::PostComplete(70000,10000,1000));
  Check(FlightLayout::PreCapacity==200000&&FlightLayout::PostCapacity==350000&&FlightLayout::PreCapacity+FlightLayout::PostCapacity==550000);
 }
 {using SweetDisplay::TransportContent::CounterOnly;
  Check(CounterOnly(true,5,4,true));Check(!CounterOnly(false,5,4,true));
  Check(!CounterOnly(true,4,4,true));Check(!CounterOnly(true,5,4,false));
  for(uint32_t reason:{1u,2u,8u,16u,32u,12u,20u,36u})Check(!CounterOnly(true,5,reason,true));
  auto first=Example();first.payload.back()=0x22;Put32(first.payload.data()+44,Crc32(first.payload.data()+FrameBytes,16));
  auto next=first;next.header.sequence=4;next.header.timestamp++;
  Put64(next.payload.data(),13);Put64(next.payload.data()+8,1000000100ULL);Put64(next.payload.data()+16,1);Put64(next.payload.data()+48,10000001);
  // Synthetic content markers 2082 -> 2062 are opaque payload, not wire sequence.
  first.payload.push_back(0x08);Put32(first.payload.data()+36,17);first.header.payload++;Put32(first.payload.data()+44,Crc32(first.payload.data()+FrameBytes,17));
  next.payload.back()=0x0e;next.payload.push_back(0x08);Put32(next.payload.data()+36,17);next.header.payload++;Put32(next.payload.data()+44,Crc32(next.payload.data()+FrameBytes,17));
  auto c=Ready();c.Receive(first);c.Receive(next);Check(c.frames==2&&c.GetState()==State::Ready);Check(U16(first.payload.data()+first.payload.size()-2)==2082&&U16(next.payload.data()+next.payload.size()-2)==2062);
  Reject([&]{c.Receive(next);});Check(c.duplicates==1);
  auto replay=Ready();replay.Receive(first);replay.Receive(next);Reject([&]{replay.Receive(first);});Check(replay.outOfOrder==1);
  auto wrong=Ready();wrong.Receive(first);auto duplicate=next;duplicate.header.sequence=3;Reject([&]{wrong.Receive(duplicate);});Check(wrong.duplicates==1);
 }
 auto m=Example();auto wire=Wire(m);Check(wire.size()==128);auto info=ParseFrameInfo(m.payload.data(),m.header.payload);Check(info.id==12&&info.sourceQpc==10000000&&info.width==2400&&info.height==1080&&info.flags==15);
 for(size_t a=0;a<=wire.size();++a)for(size_t b=a;b<=wire.size();++b){Parser p;int got=0;auto receive=[&](const Message& x){++got;Check(x.payload==m.payload&&x.header.sequence==3&&x.header.session==123&&x.header.timestamp==1000000001ULL);};p.Feed(wire.data(),a,receive);p.Feed(wire.data()+a,b-a,receive);p.Feed(wire.data()+b,wire.size()-b,receive);p.End();Check(got==1);}
 for(size_t chunk=1;chunk<=wire.size();++chunk){Parser p;int got=0;for(size_t i=0;i<wire.size();i+=chunk)p.Feed(wire.data()+i,std::min(chunk,wire.size()-i),[&](auto&){++got;});Check(got==1);}
 std::vector<uint8_t> combined;for(int i=0;i<7;++i)combined.insert(combined.end(),wire.begin(),wire.end());for(size_t split=0;split<=combined.size();++split){Parser p;int got=0;p.Feed(combined.data(),split,[&](auto&){++got;});p.Feed(combined.data()+split,combined.size()-split,[&](auto&){++got;});p.End();Check(got==7);}
 for(size_t n=1;n<wire.size();++n){Parser p;p.Feed(wire.data(),n,[](auto&){});Reject([&]{p.End();});Reject([&]{p.Feed(wire.data(),wire.size(),[](auto&){});});}
 for(auto offset:{0,4,6,8,10,16,20}){auto bad=wire;bad[size_t(offset)]^=0x80;Parser p;Reject([&]{p.Feed(bad.data(),bad.size(),[](auto&){});});Check(p.PeakPayload==0);}
 for(uint32_t n:{0u,1u,64u,MaxPayload+1,UINT32_MAX,0x80000000u}){auto bad=wire;Put32(bad.data()+12,n);Parser p;Reject([&]{p.Feed(bad.data(),bad.size(),[](auto&){});});Check(p.PeakPayload==0);}
 for(auto offset:{24,28,32,36,40,56}){auto bad=wire;Put32(bad.data()+HeaderBytes+offset,UINT32_MAX);Parser p;Reject([&]{p.Feed(bad.data(),bad.size(),[](auto&){});});Check(p.PeakPayload==0);}
 {auto big=m;big.payload.resize(MaxPayload,0x66);Put32(big.payload.data()+36,MaxAu);big.header.payload=MaxPayload;auto bytes=Wire(big);Parser p;int got=0;p.Feed(bytes.data(),bytes.size(),[&](auto& x){Check(x.payload.size()==MaxPayload);++got;});Check(got==1&&p.PeakPayload==MaxPayload);}
 Reject([]{Nanoseconds(UINT64_MAX,1);});Reject([]{Nanoseconds(10,0);});Reject([]{Nanoseconds(10,UINT64_MAX);});
 {auto c=Ready();c.Receive(m);Check(c.frames==1&&c.frameBytes==16);Reject([&]{c.Receive(m);});Check(c.duplicates==1&&c.GetState()==State::Closed);}
 {auto c=Ready();auto bad=m;bad.header.sequence=4;Reject([&]{c.Receive(bad);});Check(c.sequenceGaps==1);}
 {auto c=Ready();auto bad=m;bad.header.sequence=1;Reject([&]{c.Receive(bad);});Check(c.outOfOrder==1);}
 {Connection c(Role::Device);Reject([&]{c.Receive(m);});}
 {auto c=Ready();auto bad=m;bad.header.session++;Reject([&]{c.Receive(bad);});}
 {auto c=Ready();auto bad=m;bad.payload.back()^=1;Reject([&]{c.Receive(bad);});}
 {auto c=Ready();auto bad=m;Put32(bad.payload.data()+40,8);Reject([&]{c.Receive(bad);});}
 {auto c=Ready();c.Receive(m);auto bad=m;bad.header.sequence=4;Reject([&]{c.Receive(bad);});}
 {Connection d(Role::Device);auto h=Message{{Type::Hello,24,42,1,1},Hello(Role::Host,1,2)};Reject([&]{d.Receive(h);});}
 {Connection d(Role::Device);auto h=Message{{Type::Hello,24,42,1,1},Hello(Role::Host,0,99)};d.Receive(h);Check(d.GetState()==State::Capabilities);}
 {Caps extended{};extended.features=FeatureVideo|FeatureTouch;extended.extension=TouchDescriptor;Connection h(Role::Host,123,TouchProfileMinor,extended),d(Role::Device,0,TouchProfileMinor,extended);auto a=h.Make(Type::Hello,h.LocalHello(),1);d.Receive(a);a=d.Make(Type::Hello,d.LocalHello(),1);h.Receive(a);auto hc=h.Make(Type::Capabilities,h.LocalCapabilities(),2);auto dc=d.Make(Type::Capabilities,d.LocalCapabilities(),2);h.Receive(dc);d.Receive(hc);Check(h.TouchNegotiated()&&d.TouchNegotiated()&&h.NegotiatedMinor()==1);SweetDisplay::Touch::Configuration cfg{};cfg.targetToken=9;auto config=SweetDisplay::Touch::Configure(cfg);auto ready=SweetDisplay::Touch::Configure(cfg,SweetDisplay::Touch::Operation::Ready);Check(SweetDisplay::Touch::ParseConfiguration(config.data(),SweetDisplay::Touch::Operation::Configure).targetToken==9);Check(SweetDisplay::Touch::ParseConfiguration(ready.data(),SweetDisplay::Touch::Operation::Ready).width==2400);SweetDisplay::Touch::ContactState state(cfg);SweetDisplay::Touch::Event down{0,SweetDisplay::Touch::Action::Down,0,65535,512,uint16_t(SweetDisplay::Touch::EventPressureValid|SweetDisplay::Touch::EventPrimary|(1<<SweetDisplay::Touch::EventRotationShift)),1,10};state.Apply(down);auto move=down;move.action=SweetDisplay::Touch::Action::Move;move.x=32768;move.deviceTimestampNs=11;state.Apply(move);auto up=move;up.action=SweetDisplay::Touch::Action::Up;up.activeMask=0;up.deviceTimestampNs=12;state.Apply(up);Check(state.ActiveMask()==0);Reject([&]{state.Apply(up);});
  // A TOUCH record from the retired connection must never cross into a fresh session.
  d.Receive(h.Make(Type::Touch,config,3));auto stale=h.Make(Type::Touch,SweetDisplay::Touch::Contact(down),4);
  Connection freshHost(Role::Host,456,TouchProfileMinor,extended),freshDevice(Role::Device,0,TouchProfileMinor,extended);a=freshHost.Make(Type::Hello,freshHost.LocalHello(),1);freshDevice.Receive(a);a=freshDevice.Make(Type::Hello,freshDevice.LocalHello(),1);freshHost.Receive(a);auto freshHostCaps=freshHost.Make(Type::Capabilities,freshHost.LocalCapabilities(),2);auto freshDeviceCaps=freshDevice.Make(Type::Capabilities,freshDevice.LocalCapabilities(),2);freshHost.Receive(freshDeviceCaps);freshDevice.Receive(freshHostCaps);freshDevice.Receive(freshHost.Make(Type::Touch,config,3));Reject([&]{freshDevice.Receive(stale);});}
 {Caps extended{};extended.features=FeatureVideo|FeatureTouch;extended.extension=TouchDescriptor;Connection newer(Role::Host,123,TouchProfileMinor,extended),legacy(Role::Device);auto a=newer.Make(Type::Hello,newer.LocalHello(),1);legacy.Receive(a);a=legacy.Make(Type::Hello,legacy.LocalHello(),1);newer.Receive(a);auto nc=newer.Make(Type::Capabilities,newer.LocalCapabilities(),2);auto lc=legacy.Make(Type::Capabilities,legacy.LocalCapabilities(),2);newer.Receive(lc);legacy.Receive(nc);Check(!newer.TouchNegotiated()&&newer.NegotiatedMinor()==0);}
 {Parser old;old.Feed(wire.data(),55,[](auto&){});Reject([&]{old.End();});auto fresh=Ready();fresh.Receive(m);Check(fresh.frames==1);}
 {using namespace SweetDisplay::Transport;BoundedQueue<Message> q;for(int i=0;i<3;++i)Check(q.Push(m));for(int i=0;i<10000;++i)Check(!q.Push(m)&&q.Size()==3&&q.Bytes()==240);for(int i=0;i<3;++i)Check(q.Pop().payload==m.payload);Check(q.Size()==0&&q.peak==3);ResyncGate g;Check(g.Inspect(7)==Admission::Disconnected);g.Connected();Check(g.Inspect(0)==Admission::Resync&&g.Inspect(1)==Admission::Resync&&g.Inspect(3)==Admission::Resync&&g.Inspect(7)==Admission::Accept);g.Commit();Check(g.Inspect(0)==Admission::Accept);g.Lost();Check(g.Inspect(0)==Admission::Disconnected);g.Connected();for(int i=0;i<10000;++i)Check(g.Inspect(0)==Admission::Resync);Check(g.Inspect(7)==Admission::Accept);g.Commit();Check(g.Inspect(0)==Admission::Accept);}
 std::mt19937 random(0x53574450);for(int test=0;test<20000;++test){auto bad=wire;for(int changes=0;changes<1+test%5;++changes)bad[random()%bad.size()]=uint8_t(random());Parser p;try{p.Feed(bad.data(),bad.size(),[](auto& x){if(x.header.type==Type::Frame){ParseFrameInfo(x.payload.data(),x.header.payload);}});p.End();}catch(const Violation&){}Check(p.PeakPayload<=MaxPayload);}
 printf("PASS %llu deterministic protocol/parser/queue/resync checks\n",checks);return 0;
 }catch(const std::exception& e){fprintf(stderr,"FAIL after %llu checks: %s\n",checks,e.what());return 1;}}
