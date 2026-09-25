// PHASE 3C transport endpoint only. No decoder, renderer, USB or input injection.
#include "../protocol/TransportSender.h"
#include <cwchar>
namespace T=SweetDisplay::Transport;
namespace P=SweetDisplay::Protocol;
int wmain(int argc,wchar_t** argv){try{
 if(argc!=6)throw std::runtime_error("usage: Simulator private-output port seconds read-delay-ms capture-AUs(0/1)");
 auto number=[](const wchar_t* s){wchar_t* end=nullptr;auto n=wcstoul(s,&end,10);P::Require(s!=end&&!*end,"numeric argument");return n;};
 auto port=number(argv[2]),seconds=number(argv[3]),delay=number(argv[4]),capture=number(argv[5]);P::Require(port>=1024&&port<=65535&&seconds>=1&&seconds<=600&&delay<=2000&&capture<=1,"bounded endpoint arguments");
 std::wstring dir=std::filesystem::weakly_canonical(argv[1]).wstring()+L"\\";wchar_t exe[32768]{};P::Require(GetModuleFileNameW(nullptr,exe,32768)!=0,"executable path");auto root=std::filesystem::weakly_canonical(std::filesystem::path(exe).parent_path().parent_path().parent_path()/L"docs/evidence/private/phase3c").wstring()+L"\\";P::Require(dir.size()>root.size()&&!_wcsnicmp(dir.c_str(),root.c_str(),root.size()),"private PHASE3C output required");
 T::Winsock winsock;T::EvidenceFile frames(dir+L"received.csv"),events(dir+L"sessions.csv"),messages(dir+L"protocol-messages.csv");uint64_t messageRows=0;T::Channel::LedgerHeader(messages.value);messages.Flush();std::unique_ptr<T::EvidenceFile> aus;if(capture)aus=std::make_unique<T::EvidenceFile>(dir+L"access-units.bin",L"wb");
 fprintf(frames.value,"session,sequence,frame_id,source_qpc,frequency,source_ns,pts,width,height,flags,bytes,crc,send_ns,receive_ns,latency_ns,latency_valid\n");fprintf(events.value,"event,session,time_ns,frames,value\n");frames.Flush();events.Flush();
 T::Listener listener{uint16_t(port)};uint64_t began=T::Now(),limit=began+uint64_t(seconds)*1000000000ULL,totalFrames=0,totalBytes=0,totalRx=0,totalTx=0,totalMessages=0,connections=0,protocolErrors=0,truncated=0,ioErrors=0;size_t maxPayload=0;bool drained=false;
 while(T::Now()<limit&&!drained){auto socket=listener.Accept();if(!socket){Sleep(10);continue;}T::Channel channel(*socket,messages.value,&messageRows);P::Connection connection(P::Role::Device);
  try{
   T::DeviceHandshake(channel,connection);++connections;fprintf(events.value,"READY,%llu,%llu,0,0\n",connection.Session(),T::Now());events.Flush();
   for(;;){P::Require(T::Now()<limit,"endpoint duration expired");if(delay)Sleep(delay);auto m=channel.Receive(std::min(limit,T::Now()+3000000000ULL));connection.Receive(m);
    if(m.header.type==P::Type::Frame){auto f=P::ParseFrameInfo(m.payload.data(),m.header.payload);auto now=T::Now();bool sharedClock=connection.peer.clock==1;if(sharedClock)P::Require(now>=m.header.timestamp,"local timestamp domain");++totalFrames;totalBytes+=f.bytes;maxPayload=std::max(maxPayload,m.payload.size());P::Require(totalFrames<=40000,"bounded telemetry rows");
     fprintf(frames.value,"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%llu,%llu,%llu,%u\n",connection.Session(),m.header.sequence,f.id,f.sourceQpc,f.frequency,f.sourceNs,f.pts,f.width,f.height,f.flags,f.bytes,f.crc,m.header.timestamp,now,sharedClock?now-m.header.timestamp:0,uint32_t(sharedClock));frames.Flush();
     if(aus){P::Require(totalBytes+totalFrames*12<=512ULL*1024*1024,"bounded AU evidence exhausted");uint8_t head[12]{};P::Put32(head,f.bytes);P::Put64(head+4,f.pts);P::Require(fwrite(head,1,12,aus->value)==12&&fwrite(m.payload.data()+P::FrameBytes,1,f.bytes,aus->value)==f.bytes,"AU evidence write");aus->Flush();}
     std::vector<uint8_t> ack(32);P::Put64(ack.data(),connection.frames);P::Put64(ack.data()+8,m.header.sequence);P::Put64(ack.data()+16,f.id);P::Put64(ack.data()+24,connection.frameBytes);channel.Send(connection.Make(P::Type::Telemetry,std::move(ack),T::Now()),T::Now()+1000000000ULL);
    }else if(m.header.type==P::Type::Heartbeat){channel.Send(connection.Make(P::Type::Heartbeat,m.payload,T::Now()),T::Now()+1000000000ULL);}
    else if(m.header.type==P::Type::Control){P::Require(P::U32(m.payload.data())==1,"DRAIN request required");std::vector<uint8_t>b(8);P::Put32(b.data(),2);channel.Send(connection.Make(P::Type::Control,std::move(b),T::Now()),T::Now()+1000000000ULL);drained=true;break;}
   }
  }catch(const T::IoError& e){++ioErrors;if(channel.Partial())++truncated;fprintf(events.value,"DISCONNECTED,%llu,%llu,%llu,%d\n",connection.Session(),T::Now(),connection.frames,e.code);events.Flush();}
  catch(const P::Violation& e){++protocolErrors;fprintf(events.value,"PROTOCOL_ERROR,%llu,%llu,%llu,0\n",connection.Session(),T::Now(),connection.frames);events.Flush();fprintf(stderr,"protocol error: %s\n",e.what());}
  totalRx+=channel.bytesRead;totalTx+=channel.bytesWritten;totalMessages+=channel.messagesRead;
 }
 T::EvidenceFile result(dir+L"receiver-result.json");double elapsed=double(T::Now()-began)/1e9;fprintf(result.value,"{\"frames\":%llu,\"au_bytes\":%llu,\"bytes_received\":%llu,\"bytes_sent\":%llu,\"messages_received\":%llu,\"connections\":%llu,\"reconnects\":%llu,\"protocol_errors\":%llu,\"truncated\":%llu,\"socket_errors\":%llu,\"peak_payload\":%zu,\"seconds\":%.9f,\"receiver_fps\":%.6f,\"drained\":%s}\n",totalFrames,totalBytes,totalRx,totalTx,totalMessages,connections,connections?connections-1:0,protocolErrors,truncated,ioErrors,maxPayload,elapsed,totalFrames/elapsed,drained?"true":"false");result.Flush();P::Require(drained&&totalFrames&&!protocolErrors,"endpoint acceptance");printf("PASS transport endpoint frames=%llu bytes=%llu sessions=%llu\n",totalFrames,totalBytes,connections);return 0;
 }catch(const std::exception& e){fprintf(stderr,"Simulator ERROR: %s\n",e.what());return 1;}}
