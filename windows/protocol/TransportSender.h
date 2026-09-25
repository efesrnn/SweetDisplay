// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include "TcpStream.h"
#include "EncodedSink.h"
#include "BoundedQueue.h"
#include "ResyncGate.h"
#include "TouchProfile.h"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <cstdio>
#include <filesystem>
namespace SweetDisplay::Transport {
struct EvidenceFile {
 FILE* value=nullptr;
 EvidenceFile(const std::wstring& path,const wchar_t* mode=L"w"){if(std::filesystem::exists(path)||_wfopen_s(&value,path.c_str(),mode)||!value)throw std::runtime_error("refusing existing/unwritable transport evidence");}
 ~EvidenceFile(){if(value)fclose(value);}EvidenceFile(const EvidenceFile&)=delete;
 void Flush(){if(fflush(value)||ferror(value))throw std::runtime_error("transport evidence write failure");}
};
class Sender final:public P::EncodedSink {
 struct Item {P::FrameInfo frame;uint64_t enqueued=0;std::vector<uint8_t> payload;};
 Winsock winsock;uint16_t port;std::wstring directory;EvidenceFile records,sessions,messages;uint64_t messageRows=0,recordRowLimit=100000,messageRowLimit=150000;SweetDisplay::Touch::Endpoint* touch=nullptr;bool reducedEvidence=false;
 std::mutex mutex;std::condition_variable wake;std::thread worker;
 std::atomic<bool> stop{false},abort{false},finish{false};BoundedQueue<Item> queue;ResyncGate gate;
 std::string fatal;P::Caps limits;uint64_t activeSession=0,begin=Now(),rows=0;
 uint64_t seen=0,admitted=0,sent=0,acked=0,notReady=0,resync=0,overflow=0,flushed=0,unconfirmed=0,expired=0,connections=0,ioErrors=0,protocolErrors=0,txBytes=0,rxBytes=0,txMessages=0,rxMessages=0,touchMessages=0;
 void Record(const char* stage,const char* decision,const Item& x,uint64_t session,uint64_t sequence=0,uint64_t sending=0,uint64_t acknowledged=0){
  const auto recordBegin=MicroTiming::Begin();if(++rows>recordRowLimit)throw std::runtime_error("bounded transport log exhausted");const auto& f=x.frame;
  fprintf(records.value,"%s,%s,%llu,%llu,%llu,%llu,%llu,%u,%u,%llu,%llu,%llu\n",stage,decision,session,sequence,f.id,f.sourceQpc,f.pts,f.bytes,f.crc,x.enqueued,sending,acknowledged);if(!reducedEvidence){const auto flushBegin=MicroTiming::Begin();records.Flush();MicroTiming::End(MicroTiming::Event::TransportFlush,flushBegin,f.id,session);}MicroTiming::End(MicroTiming::Event::TransportEvidence,recordBegin,f.id,session,reducedEvidence?0u:1u,0);
 }
 void FlushQueue(){while(queue.Size()){auto x=queue.Pop();++flushed;Record("terminal","queue_aborted",x,activeSession);}}
 void RunSession(TcpStream& stream){
  P::Caps own{};if(touch){own.features=P::FeatureVideo|P::FeatureTouch;own.extension=P::TouchDescriptor;}Channel channel(stream,messages.value,&messageRows,!reducedEvidence,messageRowLimit);P::Connection connection(P::Role::Host,SessionId(),touch?P::TouchProfileMinor:0,own);Item current{};bool active=false,attempted=false;uint64_t sequence=0,sending=0,localAck=0,localBytes=0;
  auto close=[&]{if(touch)touch->End();std::lock_guard<std::mutex> lock(mutex);gate.Lost();if(active){if(attempted){++unconfirmed;Record("terminal","unconfirmed",current,connection.Session(),sequence,sending);}else{++flushed;Record("terminal","queue_aborted",current,connection.Session());}}FlushQueue();txBytes+=channel.bytesWritten;rxBytes+=channel.bytesRead;txMessages+=channel.messagesWritten;rxMessages+=channel.messagesRead;fprintf(sessions.value,"CLOSED,%llu,%llu,%llu,%llu,%llu,%llu\n",connection.Session(),Now(),localAck,channel.messagesWritten,channel.bytesWritten,channel.bytesRead);sessions.Flush();};
  try{
   HostHandshake(channel,connection);
   if(touch){P::Require(connection.TouchNegotiated(),"touch profile not negotiated");auto configuration=touch->CurrentConfiguration();auto deadline=Now()+2000000000ULL;channel.Send(connection.Make(P::Type::Touch,SweetDisplay::Touch::Configure(configuration),Now()),deadline);auto ready=channel.Receive(deadline);connection.Receive(ready);P::Require(ready.header.type==P::Type::Touch,"touch READY required");auto confirmed=SweetDisplay::Touch::ParseConfiguration(ready.payload.data(),SweetDisplay::Touch::Operation::Ready);P::Require(confirmed.coordinateMaximum==configuration.coordinateMaximum&&confirmed.width==configuration.width&&confirmed.height==configuration.height&&confirmed.maxContacts==configuration.maxContacts&&confirmed.flags==configuration.flags&&confirmed.targetToken==configuration.targetToken,"touch READY mismatch");touch->Begin(connection.Session(),configuration);}
    {std::lock_guard<std::mutex> lock(mutex);activeSession=connection.Session();limits=connection.peer;gate.Connected();++connections;fprintf(sessions.value,"READY,%llu,%llu,0,%llu,%llu,%llu\n",activeSession,Now(),channel.messagesWritten,channel.bytesWritten,channel.bytesRead);sessions.Flush();}
   auto receiveExpected=[&](P::Type expected,uint64_t deadline){for(;;){auto response=channel.Receive(deadline);connection.Receive(response);if(response.header.type==P::Type::Touch){P::Require(touch,"unexpected touch message");touch->Accept(connection.Session(),SweetDisplay::Touch::ParseEvent(response.payload.data()));++touchMessages;continue;}P::Require(response.header.type==expected,"unexpected response type");return response;}};
   uint64_t heartbeat=Now();
   for(;;){
    if(stop||abort)throw IoError("transport reset",WSAEINTR);
    // The transport worker is the sole owner of the Windows touch state machine.
    // It services bounded keepalive updates even when no new Android event arrives.
    if(touch)touch->Tick();
     {std::unique_lock<std::mutex> lock(mutex,std::defer_lock);const auto lockBegin=MicroTiming::Begin();const bool contended=!lock.try_lock();if(contended)lock.lock();MicroTiming::End(MicroTiming::Event::WorkerQueueMutexWait,lockBegin,0,0,contended?1u:0u,0);const auto holdBegin=MicroTiming::Begin();if(queue.Size()){current=queue.Pop();active=true;attempted=false;MicroTiming::End(MicroTiming::Event::WorkerQueueMutexHold,holdBegin);}else if(!finish){MicroTiming::End(MicroTiming::Event::WorkerQueueMutexHold,holdBegin);const auto waitBegin=MicroTiming::Begin();wake.wait_for(lock,std::chrono::milliseconds(10));MicroTiming::End(MicroTiming::Event::WorkerConditionWait,waitBegin);}else MicroTiming::End(MicroTiming::Event::WorkerQueueMutexHold,holdBegin);}
     if(active){
      MicroTiming::Context trace(current.frame.id,connection.Session());
      sending=Now();if(sending-current.enqueued>250000000ULL){std::lock_guard<std::mutex> lock(mutex);++expired;throw IoError("queue age deadline",-2);}
      auto message=connection.Make(P::Type::Frame,std::move(current.payload),sending);sequence=message.header.sequence;attempted=true;
      const auto sendBegin=MicroTiming::Begin();channel.Send(message,sending+1000000000ULL);MicroTiming::End(MicroTiming::Event::LogicalFrameSend,sendBegin,MicroTiming::currentFrame,MicroTiming::currentSession,current.frame.bytes,0);
      {MicroTiming::TimedMutex lock(mutex,MicroTiming::Event::SentMutexWait,MicroTiming::Event::SentMutexHold);++sent;Record("wire","sent",current,connection.Session(),sequence,sending);}
      const auto ackBegin=MicroTiming::Begin();auto response=receiveExpected(P::Type::Telemetry,sending+1000000000ULL);MicroTiming::End(MicroTiming::Event::AckReceive,ackBegin);const auto processBegin=MicroTiming::Begin();auto b=response.payload.data();P::Require(P::U64(b)==localAck+1&&P::U64(b+8)==sequence&&P::U64(b+16)==current.frame.id&&P::U64(b+24)==localBytes+current.frame.bytes,"FRAME acknowledgement accounting");
      ++localAck;localBytes+=current.frame.bytes;MicroTiming::End(MicroTiming::Event::AckProcess,processBegin);{MicroTiming::TimedMutex lock(mutex,MicroTiming::Event::AckMutexWait,MicroTiming::Event::AckMutexHold);++acked;Record("terminal","acked",current,connection.Session(),sequence,sending,Now());}active=false;current=Item{};
    }else if(finish){
     std::vector<uint8_t> b(8);P::Put32(b.data(),1);auto deadline=Now()+1000000000ULL;channel.Send(connection.Make(P::Type::Control,std::move(b),Now()),deadline);auto r=receiveExpected(P::Type::Control,deadline);P::Require(P::U32(r.payload.data())==2,"DRAIN acknowledgement");close();return;
    }else if(Now()-heartbeat>=1000000000ULL){std::vector<uint8_t>b(8);P::Put64(b.data(),Now());auto deadline=Now()+1000000000ULL;channel.Send(connection.Make(P::Type::Heartbeat,b,Now()),deadline);auto r=receiveExpected(P::Type::Heartbeat,deadline);P::Require(r.payload==b,"heartbeat echo");heartbeat=Now();}
   }
  }catch(...){close();throw;}
 }
 void Work(){
  while(!stop){
   if(finish)break;abort=false;
   try{auto stream=TcpStream::Connect(port,&abort);RunSession(*stream);if(finish)break;}
   catch(const IoError& e){std::lock_guard<std::mutex> lock(mutex);gate.Lost();if(e.code!=WSAEINTR&&e.code!=-2)++ioErrors;fprintf(sessions.value,"IO_ERROR,%llu,%llu,%d,0,0,0\n",activeSession,Now(),e.code);sessions.Flush();}
   catch(const std::exception& e){std::lock_guard<std::mutex> lock(mutex);gate.Lost();++protocolErrors;fatal=e.what();break;}
   std::unique_lock<std::mutex> lock(mutex);wake.wait_for(lock,std::chrono::milliseconds(100));
  }
  std::lock_guard<std::mutex> lock(mutex);gate.Lost();FlushQueue();
 }
public:
  Sender(uint16_t p,const std::wstring& dir,SweetDisplay::Touch::Endpoint* endpoint=nullptr,bool reduced=false,uint64_t maximumRecordRows=100000,uint64_t maximumMessageRows=150000):port(p),directory(dir),records(dir+L"/transport-frames.csv"),sessions(dir+L"/transport-sessions.csv"),messages(dir+L"/protocol-messages.csv"),recordRowLimit(maximumRecordRows),messageRowLimit(maximumMessageRows),touch(endpoint),reducedEvidence(reduced){
  Channel::LedgerHeader(messages.value);messages.Flush();
  fprintf(records.value,"stage,decision,session,sequence,frame_id,source_qpc,pts,bytes,crc,enqueue_ns,send_ns,ack_ns\n");fprintf(sessions.value,"event,session,time_ns,value,messages_sent,bytes_sent,bytes_received\n");records.Flush();sessions.Flush();worker=std::thread([this]{Work();});
 }
 ~Sender(){stop=true;abort=true;wake.notify_all();if(worker.joinable())worker.join();}
 void Consume(const P::EncodedView& v)override{
  MicroTiming::Context trace(v.id,0);const auto totalBegin=MicroTiming::Begin();MicroTiming::TimedMutex lock(mutex,MicroTiming::Event::ConsumeMutexWait,MicroTiming::Event::ConsumeMutexHold,v.id,0);if(!fatal.empty())throw std::runtime_error(fatal);P::Require(v.bytes&&v.bytes<=P::MaxAu,"encoded AU hard limit");
  const auto constructBegin=MicroTiming::Begin();Item x; x.frame={v.id,P::Nanoseconds(v.sourceQpc,v.frequency),v.pts,1,v.width,v.height,v.bytes,v.flags,P::Crc32(v.data,v.bytes),v.sourceQpc,v.frequency};x.enqueued=Now();++seen;
  auto decision=gate.Inspect(v.flags);if(decision!=Admission::Accept){if(decision==Admission::Disconnected)++notReady;else ++resync;Record("input",decision==Admission::Disconnected?"disconnected":"resync",x,activeSession);MicroTiming::End(MicroTiming::Event::ConsumeTotal,totalBegin,v.id,activeSession);return;}
  P::Require(v.bytes<=limits.maxAu&&v.width<=limits.width&&v.height<=limits.height,"receiver negotiated limits");
  if(queue.Size()>=decltype(queue)::Capacity){++overflow;Record("input","overflow",x,activeSession);gate.Lost();abort=true;FlushQueue();wake.notify_all();MicroTiming::End(MicroTiming::Event::ConsumeTotal,totalBegin,v.id,activeSession);return;}
  auto metadata=P::SerializeFrameInfo(x.frame);P::ParseFrameInfo(metadata.data(),P::FrameBytes+v.bytes);x.payload.assign(metadata.begin(),metadata.end());x.payload.insert(x.payload.end(),v.data,v.data+v.bytes);MicroTiming::End(MicroTiming::Event::FrameConstruction,constructBegin,v.id,activeSession,v.bytes,x.frame.crc);
  Record("input","admitted",x,activeSession);const auto admissionBegin=MicroTiming::Begin();P::Require(queue.Push(std::move(x)),"bounded queue admission invariant");++admitted;gate.Commit();wake.notify_all();MicroTiming::End(MicroTiming::Event::QueueAdmission,admissionBegin,v.id,activeSession,uint32_t(queue.Size()),0);MicroTiming::End(MicroTiming::Event::ConsumeTotal,totalBegin,v.id,activeSession);
 }
 void Finish(){
  finish=true;wake.notify_all();if(worker.joinable())worker.join();records.Flush();messages.Flush();double seconds=double(Now()-begin)/1e9;
  EvidenceFile result(directory+L"/transport-result.json");fprintf(result.value,"{\"seen\":%llu,\"admitted\":%llu,\"wire_complete\":%llu,\"acked\":%llu,\"disconnected\":%llu,\"resync_skipped\":%llu,\"queue_overflow\":%llu,\"queue_aborted\":%llu,\"unconfirmed\":%llu,\"queue_expired\":%llu,\"queue_peak\":%zu,\"queue_bytes_peak\":%zu,\"connections\":%llu,\"socket_errors\":%llu,\"protocol_errors\":%llu,\"messages_sent\":%llu,\"messages_received\":%llu,\"touch_messages\":%llu,\"bytes_sent\":%llu,\"bytes_received\":%llu,\"seconds\":%.9f,\"sender_fps\":%.6f,\"pending\":%zu}\n",seen,admitted,sent,acked,notReady,resync,overflow,flushed,unconfirmed,expired,queue.peak,queue.peakBytes,connections,ioErrors,protocolErrors,txMessages,rxMessages,touchMessages,txBytes,rxBytes,seconds,sent/seconds,queue.Size());result.Flush();
  P::Require(seen==admitted+notReady+resync+overflow&&admitted==acked+flushed+unconfirmed&&!queue.Size(),"transport exact accounting");if(!fatal.empty())throw std::runtime_error(fatal);P::Require(acked>0,"no validated transport frames");
 }
};
}
