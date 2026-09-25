// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
// Opt-in, bounded QPC micro-timing and PERF2 degradation flight recording.
#pragma once
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <atomic>
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <mutex>
#include <stdexcept>
#include <vector>

namespace SweetDisplay::MicroTiming {
enum class Event : uint32_t {
 HostIteration=1, HostFrameAcquire=2, HostAdmission=3, KeyedMutexWait=4,
 HostSampleDiagnostic=5, DriverAck=6, HostEvidence=7, HostReturn=8,
 EncoderPump=10, EncoderEventService=11, EncoderOutputTotal=12,
 EncoderProcessOutput=13, EncoderOutputParse=14, EncoderEvidenceWrite=15,
 SinkConsume=16, ConversionSubmit=17, ConversionWait=18,
 EncoderProcessInput=19, EncoderSubmission=20,
 ConsumeTotal=21, ConsumeMutexWait=22, ConsumeMutexHold=23,
 FrameConstruction=24, TransportEvidence=25, QueueAdmission=26,
 WorkerQueueMutexWait=30, WorkerQueueMutexHold=31, WorkerConditionWait=32,
 LogicalFrameSend=33, SocketWaitWrite=34, SocketWriteCall=35,
 ChannelTxEvidence=36, AckReceive=37, SocketWaitRead=38,
 SocketReadCall=39, ChannelRxEvidence=40, SentMutexWait=41,
 SentMutexHold=42, AckMutexWait=43, AckMutexHold=44, AckProcess=45,
 SocketConfiguration=46, ChannelSend=47, ChannelReceive=48,
 SocketWriteResult=49, SocketReadResult=50, TransportFlush=51,
 ChannelFlush=52, DetectorTrigger=60
};

struct Record {
 uint64_t start=0,duration=0,frame=0,session=0;
 uint32_t event=0,thread=0,value0=0,value1=0;
};
static_assert(sizeof(Record)==48,"stable micro-timing record size");

inline uint64_t Counter(){LARGE_INTEGER q{};QueryPerformanceCounter(&q);return uint64_t(q.QuadPart);}

struct FlightLayout {
 static constexpr uint64_t PreCapacity=200000,PostCapacity=350000,NoTrigger=UINT64_MAX;
 static uint64_t Index(uint64_t sequence,uint64_t trigger){return trigger==NoTrigger||sequence<trigger?sequence%PreCapacity:PreCapacity+(sequence-trigger);}
 static uint64_t PreBegin(uint64_t trigger){return trigger>PreCapacity?trigger-PreCapacity:0;}
 static uint64_t PostEnd(uint64_t next,uint64_t trigger){return (std::min)(next,trigger+PostCapacity);}
 static bool PostComplete(uint64_t now,uint64_t triggerQpc,uint64_t frequency,uint32_t seconds=60){return triggerQpc&&now>=triggerQpc+uint64_t(seconds)*frequency;}
};

class DegradationDetector {
public:
 struct Decision {uint64_t qpc=0,source=0,host=0;double seconds=0,sourceFps=0,hostFps=0,ratio=0;};
 static constexpr double WindowSeconds=10.0,MinimumSourceFps=45.0,RatioThreshold=0.75;
private:
 struct Sample {uint64_t qpc=0,source=0,host=0;};
 std::array<Sample,32> samples{};size_t count=0;bool fired=false;
public:
 bool Observe(uint64_t qpc,uint64_t source,uint64_t host,uint64_t frequency,Decision& decision){
  if(fired||!frequency)return false;
  if(count&&(qpc<=samples[count-1].qpc||source<samples[count-1].source||host<samples[count-1].host))count=0;
  if(count==samples.size()){std::move(samples.begin()+1,samples.end(),samples.begin());--count;}
  samples[count++]={qpc,source,host};
  const auto window=uint64_t(WindowSeconds*double(frequency));
  while(count>2&&qpc-samples[1].qpc>=window){std::move(samples.begin()+1,samples.begin()+count,samples.begin());--count;}
  if(count<2||qpc-samples[0].qpc<window)return false;
  const auto sourceDelta=source-samples[0].source,hostDelta=host-samples[0].host;
  const double elapsed=double(qpc-samples[0].qpc)/double(frequency);
  const double sourceRate=double(sourceDelta)/elapsed,hostRate=double(hostDelta)/elapsed;
  const double ratio=sourceDelta?double(hostDelta)/double(sourceDelta):1.0;
  if(sourceRate<MinimumSourceFps||ratio>=RatioThreshold)return false;
  fired=true;decision={qpc,source,host,elapsed,sourceRate,hostRate,ratio};return true;
 }
 bool Fired()const{return fired;}
};

class Recorder {
 static constexpr uint64_t LinearCapacity=750000,FlightCapacity=FlightLayout::PreCapacity+FlightLayout::PostCapacity;
 static constexpr size_t SummaryCount=8,SummaryBins=2001;
 struct Summary {std::atomic<uint64_t> count{0},ticks{0},maximum{0};std::array<std::atomic<uint64_t>,SummaryBins> bins{};};
 std::vector<Record> records;
 std::unique_ptr<std::atomic<uint64_t>[]> stamps;
 std::atomic<uint64_t> next{0},dropped{0};
 std::atomic<uint64_t> triggerSequence{FlightLayout::NoTrigger};
 uint64_t frequency=0,started=0;
 uint64_t triggerQpc=0,triggerSource=0,triggerHost=0;
 double triggerWindowSeconds=0,triggerSourceFps=0,triggerHostFps=0,triggerRatio=0;
 double calibrationMeanNs=0,calibrationP95Ns=0,calibrationMaxNs=0;
 std::wstring directory;
 bool reduced=false,flight=false;
 std::atomic<bool> enabled{false};
 std::array<Summary,SummaryCount> summaries{};
 static int SummaryIndex(Event event,uint64_t frame,uint32_t value0){
  if(event==Event::HostIteration&&frame)return 0;
  if(event==Event::EncoderEventService&&value0==602)return 1;
  if(event==Event::EncoderProcessInput)return 2;
  if(event==Event::EncoderProcessOutput)return 3;
  if(event==Event::ConversionWait)return 4;
  if(event==Event::SinkConsume)return 5;
  if(event==Event::SocketWriteCall)return 6;
  if(event==Event::AckReceive)return 7;
  return -1;
 }
 void Summarize(Event event,uint64_t duration,uint64_t frame,uint32_t value0){
  const int index=SummaryIndex(event,frame,value0);if(index<0||!frequency)return;auto& summary=summaries[size_t(index)];summary.count.fetch_add(1,std::memory_order_relaxed);summary.ticks.fetch_add(duration,std::memory_order_relaxed);auto prior=summary.maximum.load(std::memory_order_relaxed);while(prior<duration&&!summary.maximum.compare_exchange_weak(prior,duration,std::memory_order_relaxed)){}
  const auto bin=(std::min)(SummaryBins-1,size_t(double(duration)*10000.0/double(frequency)));summary.bins[bin].fetch_add(1,std::memory_order_relaxed);
 }
 void WriteSummaries(FILE* f){
  static constexpr const char* names[SummaryCount]={"HostFrameIteration","EncoderEventServiceHaveOutput","EncoderProcessInput","EncoderProcessOutput","ConversionWait","SinkConsume","SocketWriteCall","AckReceive"};
  fprintf(f,",\"timing_summary_bin_ms\":0.1,\"timing_summary\":{");
  for(size_t i=0;i<SummaryCount;++i){const auto count=summaries[i].count.load(),ticks=summaries[i].ticks.load(),maximum=summaries[i].maximum.load();auto percentile=[&](uint64_t numerator){if(!count)return 0.0;const auto target=(count*numerator+99)/100;uint64_t accumulated=0;for(size_t bin=0;bin<SummaryBins;++bin){accumulated+=summaries[i].bins[bin].load();if(accumulated>=target)return double(bin)/10.0;}return double(SummaryBins-1)/10.0;};fprintf(f,"%s\"%s\":{\"count\":%llu,\"mean_ms\":%.6f,\"p50_ms\":%.6f,\"p95_ms\":%.6f,\"p99_ms\":%.6f,\"max_ms\":%.6f}",i?",":"",names[i],count,count?double(ticks)*1000.0/double(frequency)/double(count):0.0,percentile(50),percentile(95),percentile(99),double(maximum)*1000.0/double(frequency));}
  fprintf(f,"}");
 }
 void Calibrate(){
  constexpr size_t Count=20000;std::vector<uint64_t> samples(Count);Record scratch{};std::atomic<uint64_t> cursor{0};
  for(size_t i=0;i<Count;++i){const auto begin=Counter();const auto slot=cursor.fetch_add(1,std::memory_order_relaxed);scratch.start=begin;scratch.event=uint32_t(Event::HostReturn);scratch.value0=uint32_t(slot);const auto end=Counter();samples[i]=end-begin;}
  std::sort(samples.begin(),samples.end());long double total=0;for(auto x:samples)total+=x;
  calibrationMeanNs=double(total/Count)*1e9/double(frequency);
  calibrationP95Ns=double(samples[(Count*95+99)/100-1])*1e9/double(frequency);
  calibrationMaxNs=double(samples.back())*1e9/double(frequency);
  if(!scratch.event)throw std::runtime_error("micro-timing calibration failure");
 }
 template<class T> static void Put(FILE* f,const T& value){if(fwrite(&value,sizeof(value),1,f)!=1)throw std::runtime_error("micro-timing write failure");}
public:
 static Recorder& Instance(){static Recorder value;return value;}
 bool Active()const{return enabled.load(std::memory_order_relaxed);}
 void Start(const std::wstring& dir,bool reducedEvidence,bool flightRecorder=false){
  if(dir.empty()||Active())throw std::runtime_error("invalid micro-timing start");
  flight=flightRecorder;const auto binary=dir+(flight?L"/flight-recorder.bin":L"/micro-timing.bin"),meta=dir+(flight?L"/flight-recorder-meta.json":L"/micro-timing-meta.json");
  if(std::filesystem::exists(binary)||std::filesystem::exists(meta))throw std::runtime_error("refusing existing micro-timing evidence");
  LARGE_INTEGER f{};QueryPerformanceFrequency(&f);frequency=uint64_t(f.QuadPart);directory=dir;reduced=reducedEvidence;
   next.store(0);dropped.store(0);triggerSequence.store(FlightLayout::NoTrigger);triggerQpc=triggerSource=triggerHost=0;triggerWindowSeconds=triggerSourceFps=triggerHostFps=triggerRatio=0;
   for(auto& summary:summaries){summary.count.store(0);summary.ticks.store(0);summary.maximum.store(0);for(auto& bin:summary.bins)bin.store(0);}
  const auto capacity=flight?FlightCapacity:LinearCapacity;records.resize(size_t(capacity));
  if(flight){stamps=std::make_unique<std::atomic<uint64_t>[]>(size_t(capacity));for(uint64_t i=0;i<capacity;++i)stamps[i].store(FlightLayout::NoTrigger,std::memory_order_relaxed);}
  Calibrate();started=Counter();enabled.store(true,std::memory_order_release);
 }
 void Emit(Event event,uint64_t begin,uint64_t frame,uint64_t session,uint32_t value0,uint32_t value1){
   if(!Active())return;const auto end=Counter(),duration=end>=begin?end-begin:0;Summarize(event,duration,frame,value0);const auto sequence=next.fetch_add(1,flight?std::memory_order_seq_cst:std::memory_order_relaxed);
   if(!flight){if(sequence>=LinearCapacity){dropped.fetch_add(1,std::memory_order_relaxed);return;}records[size_t(sequence)]={begin,duration,frame,session,uint32_t(event),GetCurrentThreadId(),value0,value1};return;}
  const auto trigger=triggerSequence.load(std::memory_order_seq_cst);
  if(trigger!=FlightLayout::NoTrigger&&sequence>=trigger+FlightLayout::PostCapacity){dropped.fetch_add(1,std::memory_order_relaxed);return;}
   const auto index=FlightLayout::Index(sequence,trigger);records[size_t(index)]={begin,duration,frame,session,uint32_t(event),GetCurrentThreadId(),value0,value1};stamps[size_t(index)].store(sequence,std::memory_order_release);
 }
 void Instant(Event event,uint64_t frame,uint64_t session,uint32_t value0=0,uint32_t value1=0){const auto q=Counter();Emit(event,q,frame,session,value0,value1);}
 bool Trigger(const DegradationDetector::Decision& decision){
  if(!flight||!Active())return false;uint64_t expected=FlightLayout::NoTrigger;const auto sequence=next.load(std::memory_order_seq_cst);
  if(!triggerSequence.compare_exchange_strong(expected,sequence,std::memory_order_seq_cst))return false;
  triggerQpc=decision.qpc;triggerSource=decision.source;triggerHost=decision.host;triggerWindowSeconds=decision.seconds;triggerSourceFps=decision.sourceFps;triggerHostFps=decision.hostFps;triggerRatio=decision.ratio;
  Instant(Event::DetectorTrigger,triggerSource,triggerHost,uint32_t(triggerRatio*1000000.0),uint32_t(triggerWindowSeconds*1000.0));return true;
 }
 bool IsFlight()const{return flight;}
 bool Triggered()const{return triggerSequence.load(std::memory_order_acquire)!=FlightLayout::NoTrigger;}
 bool PostComplete(uint64_t now)const{return flight&&Triggered()&&FlightLayout::PostComplete(now,triggerQpc,frequency);}
 void Finish(){
  if(!enabled.exchange(false,std::memory_order_acq_rel))return;const auto observed=next.load();const auto trigger=triggerSequence.load();uint64_t count=0,missing=0;std::vector<Record> ordered;
  if(flight&&trigger!=FlightLayout::NoTrigger){
   const auto preBegin=FlightLayout::PreBegin(trigger),postEnd=FlightLayout::PostEnd(observed,trigger);ordered.reserve(size_t((trigger-preBegin)+(postEnd-trigger)));
   for(uint64_t sequence=preBegin;sequence<trigger;++sequence){const auto index=FlightLayout::Index(sequence,FlightLayout::NoTrigger);if(stamps[size_t(index)].load(std::memory_order_acquire)==sequence)ordered.push_back(records[size_t(index)]);else ++missing;}
   for(uint64_t sequence=trigger;sequence<postEnd;++sequence){const auto index=FlightLayout::Index(sequence,trigger);if(stamps[size_t(index)].load(std::memory_order_acquire)==sequence)ordered.push_back(records[size_t(index)]);else ++missing;}
   count=ordered.size();
  }else if(!flight)count=(std::min)(observed,LinearCapacity);
  if(missing)dropped.fetch_add(missing,std::memory_order_relaxed);
  const auto writeBegin=Counter();FILE* f=nullptr;const auto binary=directory+(flight?L"/flight-recorder.bin":L"/micro-timing.bin");
  if(!flight||trigger!=FlightLayout::NoTrigger){
   if(_wfopen_s(&f,binary.c_str(),L"wb")||!f)throw std::runtime_error("cannot create micro-timing evidence");
   try{const uint32_t magic=0x31544d53,version=flight?2u:1u,recordSize=sizeof(Record),reducedValue=reduced?1u:0u;Put(f,magic);Put(f,version);Put(f,frequency);Put(f,started);Put(f,count);const auto lost=dropped.load();Put(f,lost);Put(f,recordSize);Put(f,reducedValue);const auto* data=flight?ordered.data():records.data();if(count&&fwrite(data,sizeof(Record),size_t(count),f)!=count)throw std::runtime_error("micro-timing record write failure");if(fflush(f)||ferror(f))throw std::runtime_error("micro-timing flush failure");fclose(f);f=nullptr;}catch(...){if(f)fclose(f);throw;}
  }
  const double writeMs=double(Counter()-writeBegin)*1000.0/double(frequency);const auto meta=directory+(flight?L"/flight-recorder-meta.json":L"/micro-timing-meta.json");
  if(_wfopen_s(&f,meta.c_str(),L"w")||!f)throw std::runtime_error("cannot create micro-timing metadata");
  if(flight){
   double preCoverage=0,postCoverage=0;if(trigger!=FlightLayout::NoTrigger&&count){const auto first=ordered.front().start,last=ordered.back().start+ordered.back().duration;if(triggerQpc>=first)preCoverage=double(triggerQpc-first)/double(frequency);if(last>=triggerQpc)postCoverage=double(last-triggerQpc)/double(frequency);}
    fprintf(f,"{\"version\":2,\"mode\":\"flight-recorder\",\"frequency\":%llu,\"observed_events\":%llu,\"records\":%llu,\"dropped\":%llu,\"pre_capacity\":%llu,\"post_capacity\":%llu,\"record_size\":%u,\"reduced_evidence\":%s,\"triggered\":%s,\"trigger_qpc\":%llu,\"trigger_sequence\":%llu,\"trigger_source\":%llu,\"trigger_host\":%llu,\"trigger_window_seconds\":%.6f,\"trigger_source_fps\":%.6f,\"trigger_host_fps\":%.6f,\"trigger_ratio\":%.9f,\"pre_coverage_seconds\":%.6f,\"post_coverage_seconds\":%.6f,\"detector_window_seconds\":10.0,\"detector_minimum_source_fps\":45.0,\"detector_ratio_threshold\":0.75,\"calibration_count\":20000,\"calibration_mean_ns\":%.3f,\"calibration_p95_ns\":%.3f,\"calibration_max_ns\":%.3f,\"trace_write_ms\":%.6f",frequency,observed,count,dropped.load(),FlightLayout::PreCapacity,FlightLayout::PostCapacity,unsigned(sizeof(Record)),reduced?"true":"false",trigger!=FlightLayout::NoTrigger?"true":"false",triggerQpc,trigger==FlightLayout::NoTrigger?0:trigger,triggerSource,triggerHost,triggerWindowSeconds,triggerSourceFps,triggerHostFps,triggerRatio,preCoverage,postCoverage,calibrationMeanNs,calibrationP95Ns,calibrationMaxNs,writeMs);
   }else fprintf(f,"{\"version\":1,\"frequency\":%llu,\"records\":%llu,\"dropped\":%llu,\"capacity\":%llu,\"record_size\":%u,\"reduced_evidence\":%s,\"calibration_count\":20000,\"calibration_mean_ns\":%.3f,\"calibration_p95_ns\":%.3f,\"calibration_max_ns\":%.3f,\"trace_write_ms\":%.6f",frequency,count,dropped.load(),LinearCapacity,unsigned(sizeof(Record)),reduced?"true":"false",calibrationMeanNs,calibrationP95Ns,calibrationMaxNs,writeMs);
   WriteSummaries(f);fprintf(f,"}\n");
  if(fclose(f))throw std::runtime_error("micro-timing metadata close failure");stamps.reset();records.clear();records.shrink_to_fit();
 }
};

inline thread_local uint64_t currentFrame=0,currentSession=0;
class Context {
 uint64_t oldFrame=0,oldSession=0;
public:
 Context(uint64_t frame,uint64_t session):oldFrame(currentFrame),oldSession(currentSession){currentFrame=frame;currentSession=session;}
 ~Context(){currentFrame=oldFrame;currentSession=oldSession;}
};
inline bool Active(){return Recorder::Instance().Active();}
inline uint64_t Begin(){return Active()?Counter():0;}
inline void End(Event event,uint64_t begin,uint64_t frame=currentFrame,uint64_t session=currentSession,uint32_t value0=0,uint32_t value1=0){if(begin)Recorder::Instance().Emit(event,begin,frame,session,value0,value1);}
inline void Instant(Event event,uint32_t value0=0,uint32_t value1=0){if(Active())Recorder::Instance().Instant(event,currentFrame,currentSession,value0,value1);}

class TimedMutex {
 std::unique_lock<std::mutex> lock;
 Event holdEvent;uint64_t holdBegin=0;uint64_t frame=0,session=0;
public:
 TimedMutex(std::mutex& mutex,Event waitEvent,Event held,uint64_t f=currentFrame,uint64_t s=currentSession):lock(mutex,std::defer_lock),holdEvent(held),frame(f),session(s){const auto begin=Begin();const bool contended=!lock.try_lock();if(contended)lock.lock();End(waitEvent,begin,frame,session,contended?1u:0u,0);holdBegin=Begin();}
 ~TimedMutex(){End(holdEvent,holdBegin,frame,session);if(lock.owns_lock())lock.unlock();}
 std::unique_lock<std::mutex>& Get(){return lock;}
 TimedMutex(const TimedMutex&)=delete;TimedMutex& operator=(const TimedMutex&)=delete;
};

class Session {
 bool active=false;
public:
 Session(const std::wstring& directory,bool reduced,bool flight=false){if(!directory.empty()){Recorder::Instance().Start(directory,reduced,flight);active=true;}}
 ~Session(){if(active)try{Recorder::Instance().Finish();}catch(...){} }
 void Finish(){if(active){Recorder::Instance().Finish();active=false;}}
};
}
