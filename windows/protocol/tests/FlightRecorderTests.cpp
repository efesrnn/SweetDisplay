#include "../MicroTiming.h"
#include <cstdio>
#include <stdexcept>
#include <vector>

using namespace SweetDisplay::MicroTiming;
static uint32_t checks=0;
static void Check(bool value,const char* message){++checks;if(!value)throw std::runtime_error(message);}

int main(){
 try{
  DegradationDetector::Decision decision{};
  DegradationDetector healthy;
  for(uint64_t second=0;second<=20;++second)Check(!healthy.Observe(second*1000,second*55,second*55,1000,decision),"healthy source/Host triggered");
  Check(!healthy.Fired(),"healthy detector state");

  DegradationDetector slowSource;
  for(uint64_t second=0;second<=20;++second)Check(!slowSource.Observe(second*1000,second*30,second*30,1000,decision),"matching slow source triggered");
  Check(!slowSource.Fired(),"matching slow-source detector state");

  DegradationDetector transient;uint64_t transientHost=0;
  for(uint64_t second=0;second<=20;++second){if(second)transientHost+=second==5?0:55;Check(!transient.Observe(second*1000,second*55,transientHost,1000,decision),"one transient interval triggered");}
  Check(!transient.Fired(),"transient detector state");

  DegradationDetector degraded;uint32_t fires=0;
  for(uint64_t second=0;second<=20;++second)if(degraded.Observe(second*1000,second*55,second*27,1000,decision))++fires;
  Check(fires==1,"sustained divergence did not trigger exactly once");
  Check(decision.qpc==10000&&decision.seconds==10.0&&decision.sourceFps==55.0&&decision.hostFps==27.0&&decision.ratio<0.50,"trigger boundary mismatch");

  constexpr uint64_t trigger=FlightLayout::PreCapacity+12,postRecords=1000;
  std::vector<int64_t> ring(size_t(FlightLayout::PreCapacity+FlightLayout::PostCapacity),-1);
  for(uint64_t sequence=0;sequence<trigger;++sequence)ring[size_t(FlightLayout::Index(sequence,FlightLayout::NoTrigger))]=int64_t(sequence);
  for(uint64_t sequence=trigger;sequence<trigger+postRecords;++sequence)ring[size_t(FlightLayout::Index(sequence,trigger))]=int64_t(sequence);
  Check(FlightLayout::PreBegin(trigger)==12,"pre-trigger begin mismatch");
  bool prePreserved=true;for(uint64_t sequence=12;sequence<trigger;++sequence)if(ring[size_t(FlightLayout::Index(sequence,FlightLayout::NoTrigger))]!=int64_t(sequence)){prePreserved=false;break;}
  Check(prePreserved,"rolling pre-trigger evidence mismatch");
  bool postPreserved=true;for(uint64_t sequence=trigger;sequence<trigger+postRecords;++sequence)if(ring[size_t(FlightLayout::Index(sequence,trigger))]!=int64_t(sequence)){postPreserved=false;break;}
  Check(postPreserved,"post-trigger evidence mismatch");
  Check(FlightLayout::PostEnd(trigger+FlightLayout::PostCapacity+5,trigger)==trigger+FlightLayout::PostCapacity,"post buffer is not bounded");
  Check(!FlightLayout::PostComplete(69999,10000,1000)&&FlightLayout::PostComplete(70000,10000,1000),"post-trigger time boundary mismatch");
  Check(FlightLayout::PreCapacity==200000&&FlightLayout::PostCapacity==350000,"production capacity mismatch");
  std::printf("PASS %u flight-recorder synthetic checks: healthy, matching-slow-source, transient, sustained-once, pre-ring, post-bound, bounded-capacity\n",checks);
  return 0;
 }catch(const std::exception& error){std::fprintf(stderr,"FAIL after %u checks: %s\n",checks,error.what());return 1;}
}
