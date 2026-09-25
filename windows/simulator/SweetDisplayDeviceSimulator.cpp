// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
// PHASE 3D executable; protected PHASE 3C endpoint and wire protocol are unchanged.
#include "VisualPipeline.h"
#include "ResourceWatch.h"
#include <thread>
#include <fstream>
#include <psapi.h>
namespace V=SweetDisplay::Visual;
namespace P=SweetDisplay::Protocol;
namespace T=SweetDisplay::Transport;
static V::ResourceWatch* resourceWatch=nullptr;
static void ResourceCheckpoint(const char* name){
    // The journal records the exact event time; the observer records when it
    // could actually sample. Coalesced reason bits are explicitly retained.
    static std::atomic<bool> submitted{false},decoded{false},presented{false};
    if(!strcmp(name,"renderer_initialized"))resourceWatch->Request(2);
    else if(!strcmp(name,"decoder_initialized")){submitted=false;decoded=false;presented=false;resourceWatch->Request(3);}
    else if(!strcmp(name,"submission_begin")&&!submitted.exchange(true))resourceWatch->Request(4);
    else if(!strcmp(name,"decoded_surface")&&!decoded.exchange(true))resourceWatch->Request(5);
    else if(!strcmp(name,"present_success")&&!presented.exchange(true))resourceWatch->Request(6);
    else if(!strcmp(name,"decoder_reset_begin"))resourceWatch->Request(7);
    else if(!strcmp(name,"decoder_released"))resourceWatch->Request(8);
}

struct Shared {
    std::mutex lock,presentationGate;
    std::atomic<uint64_t> generation{0};
    V::Queue queue;
    std::atomic<bool> stop{false},done{false},drain{false},drained{false};
    uint64_t desired=0;
    size_t peakPayload=0;
    bool reset=false;
    std::exception_ptr error;
    void Session(uint64_t session) { std::lock_guard<std::mutex> guard(lock); std::lock_guard<std::mutex> present(presentationGate); ++generation; V::Identity(session,0,generation);V::Stage("session_reset",queue.Size());queue.Reset(session); desired=session; reset=true; }
    void Push(V::Frame f) { std::lock_guard<std::mutex> guard(lock); f.generation=generation.load();V::Identity(f.session,f.info.id,f.generation);V::Stage("queue_push",queue.Size());auto old=queue.overflow;auto recovering=queue.Recovering();bool accepted=queue.Push(std::move(f));
        V::Stage(accepted?"queue_admit":queue.overflow!=old?"queue_overflow":"resync_skip",queue.Size());if(accepted&&recovering)V::Stage("idr_recovery");
        if(queue.overflow!=old){std::lock_guard<std::mutex> present(presentationGate);++generation;reset=true;} }
};
static unsigned Number(const wchar_t* s) { wchar_t* end=nullptr; auto n=wcstoul(s,&end,10); P::Require(end!=s&&!*end,"numeric argument"); return n; }
static std::wstring PrivateDirectory(const wchar_t* arg) {
    wchar_t exe[32768]{}; P::Require(GetModuleFileNameW(nullptr,exe,32768)!=0,"executable path");
    auto root=std::filesystem::weakly_canonical(std::filesystem::path(exe).parent_path().parent_path().parent_path()/L"docs/evidence/private/phase3d").wstring()+L"\\";
    auto dir=std::filesystem::weakly_canonical(arg).wstring()+L"\\";
    P::Require(dir.size()>root.size()&&!_wcsnicmp(root.c_str(),dir.c_str(),root.size()),"private PHASE3D output required"); return dir;
}
static void Tests() {
    V::StageTrace trace;auto begin=T::Now();for(size_t i=0;i<V::StageTrace::Bound+3;++i)trace.Record("test",i);
    P::Require(trace.Count()==V::StageTrace::Bound&&trace.Lost()==3,"timing bound and explicit lost count");
    printf("TRACE_TEST ns_per_record=%.2f bounded_rows=%zu lost=%llu\n",double(T::Now()-begin)/(V::StageTrace::Bound+3),trace.Count(),trace.Lost());
    V::Queue q; q.Reset(1); V::Frame f; f.session=1; f.info.flags=0;
    P::Require(!q.Push(f)&&q.resync==1,"dependent startup rejected"); f.info.flags=7;
    for(unsigned i=0;i<3;++i)P::Require(q.Push(f),"queue capacity");
    P::Require(!q.Push(f)&&q.Size()==0&&q.overflow==1&&q.resetDrops==3,"overflow invalidates dependencies");
    f.info.flags=0; P::Require(!q.Push(f),"dependent frame after overflow rejected");
    f.info.flags=7; P::Require(q.Push(f),"IDR recovery"); V::Frame out; P::Require(q.Pop(out)&&out.session==1,"queue take");
    q.Push(f); q.Reset(2); P::Require(!q.Pop(out),"no stale session queue"); bool rejected=false;
    try { q.Push(f); } catch(const P::Violation&) { rejected=true; } P::Require(rejected,"old session rejected");
    const uint8_t corrupt[]={0,0,0,1,0xE5,0}; rejected=false; try { P::NalFlags(corrupt,sizeof(corrupt)); } catch(const P::Violation&) { rejected=true; }
    P::Require(rejected,"corrupt NAL forbidden bit rejected");
    P::Require(q.received==q.admitted+q.overflow+q.resync && q.admitted==q.taken+q.resetDrops+q.Size(),"exact queue accounting");
    printf("PASS bounded queue, dependency loss, IDR gate, session reset, corrupt NAL, exact accounting\n");
}

static void Replay(Shared& state,const std::wstring& input,unsigned count,T::EvidenceFile& events) {
    // Preserved genuine encoded Display 3 fixture, not a synthetic source. Two
    // independent sessions use the same fixture only for deterministic reset tests.
    for(unsigned pass=0;pass<2&&!state.stop;++pass) {
        state.Session(pass+1); fprintf(events.value,"REPLAY_SESSION,%u,%llu\n",pass+1,T::Now()); events.Flush();
        std::ifstream file(std::filesystem::path(input),std::ios::binary); P::Require(bool(file),"replay fixture open");
        for(unsigned i=0;i<count&&!state.stop;++i) {
            uint8_t head[12]{}; file.read(reinterpret_cast<char*>(head),12); P::Require(file.gcount()==12,"fixture record header");
            uint32_t size=P::U32(head); P::Require(size&&size<=4*1024*1024,"bounded fixture AU");
            V::Frame f; f.session=pass+1; f.sequence=i+3; f.received=T::Now(); f.info.id=i+1; f.info.pts=P::U64(head+4);
            f.info.width=2400; f.info.height=1080; f.info.bytes=size; f.au.resize(size); file.read(reinterpret_cast<char*>(f.au.data()),size);
            P::Require(file.gcount()==size,"fixture AU truncated"); f.info.flags=P::NalFlags(f.au.data(),size);
            state.Push(std::move(f)); Sleep(35);
        }
        state.drain=true; auto deadline=T::Now()+3000000000ULL;
        while(!state.drained&&!state.stop&&T::Now()<deadline)Sleep(2);
        P::Require(state.drained||state.stop,"replay drain deadline"); state.drain=false; state.drained=false;
    }
}

static void Receive(Shared& state,const std::wstring& dir,unsigned port,unsigned seconds,bool capture,T::EvidenceFile& events) {
    T::Winsock winsock; T::EvidenceFile messages(dir+L"protocol-messages.csv"),frames(dir+L"received.csv");
    std::unique_ptr<T::EvidenceFile> aus;if(capture)aus=std::make_unique<T::EvidenceFile>(dir+L"access-units.bin",L"wb");
    T::Channel::LedgerHeader(messages.value); uint64_t rows=0,total=0,bytes=0,connections=0;size_t peakPayload=0;
    fprintf(frames.value,"session,sequence,frame_id,source_qpc,frequency,source_ns,pts,width,height,flags,bytes,crc,send_ns,receive_ns,latency_ns,latency_valid\n"); frames.Flush();
    T::Listener listener{uint16_t(port)}; auto deadline=T::Now()+uint64_t(seconds)*1000000000ULL;
    while(!state.stop&&T::Now()<deadline) {
        auto socket=listener.Accept(); if(!socket){Sleep(5);continue;} T::Channel channel(*socket,messages.value,&rows); P::Connection connection(P::Role::Device);
        try {
            V::Stage("handshake_begin");T::DeviceHandshake(channel,connection); state.Session(connection.Session());++connections;V::Stage("handshake_ready");if(resourceWatch)resourceWatch->Request(1);
            fprintf(events.value,"READY,%llu,%llu\n",connection.Session(),T::Now()); events.Flush();
            while(!state.stop) {
                V::Stage("receive_begin");auto m=channel.Receive(std::min(deadline,T::Now()+3000000000ULL));V::Stage("receive_end",m.header.sequence);V::Stage("parse_begin"); connection.Receive(m);V::Stage("parse_end");
                if(m.header.type==P::Type::Frame) {
                    auto f=P::ParseFrameInfo(m.payload.data(),m.header.payload); auto now=T::Now();V::Identity(connection.Session(),f.id,state.generation);V::Stage("frame_received",f.pts); ++total; bytes+=f.bytes;peakPayload=std::max(peakPayload,m.payload.size());state.peakPayload=peakPayload;
                    P::Require(total<=20000&&bytes+total*12<=512ULL*1024*1024,"bounded live evidence cap");
                    bool local=connection.peer.clock==1; P::Require(!local||now>=m.header.timestamp,"shared clock");
                    V::Stage("evidence_begin");fprintf(frames.value,"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%llu,%llu,%llu,%u\n",connection.Session(),m.header.sequence,f.id,f.sourceQpc,f.frequency,f.sourceNs,f.pts,f.width,f.height,f.flags,f.bytes,f.crc,m.header.timestamp,now,local?now-m.header.timestamp:0,unsigned(local)); frames.Flush();
                    if(aus){uint8_t head[12]{}; P::Put32(head,f.bytes); P::Put64(head+4,f.pts);
                    P::Require(fwrite(head,1,12,aus->value)==12&&fwrite(m.payload.data()+P::FrameBytes,1,f.bytes,aus->value)==f.bytes,"bounded AU evidence write"); aus->Flush();}
                    V::Stage("evidence_end");V::Frame input; input.session=connection.Session(); input.sequence=m.header.sequence; input.received=now; input.info=f;
                    input.au.assign(m.payload.begin()+P::FrameBytes,m.payload.end()); state.Push(std::move(input));
                    std::vector<uint8_t> ack(32); P::Put64(ack.data(),connection.frames); P::Put64(ack.data()+8,m.header.sequence); P::Put64(ack.data()+16,f.id); P::Put64(ack.data()+24,connection.frameBytes);
                    V::Stage("ack_begin");channel.Send(connection.Make(P::Type::Telemetry,std::move(ack),T::Now()),T::Now()+1000000000ULL);V::Stage("ack_end");
                } else if(m.header.type==P::Type::Heartbeat)channel.Send(connection.Make(P::Type::Heartbeat,m.payload,T::Now()),T::Now()+1000000000ULL);
                else if(m.header.type==P::Type::Control) {
                    P::Require(P::U32(m.payload.data())==1,"DRAIN request"); state.drain=true; auto until=T::Now()+2500000000ULL;
                    while(!state.drained&&!state.stop&&T::Now()<until)Sleep(2); P::Require(state.drained&&!state.stop,"visible decoder drain deadline");
                    std::vector<uint8_t> b(8); P::Put32(b.data(),2); channel.Send(connection.Make(P::Type::Control,std::move(b),T::Now()),T::Now()+1000000000ULL);
                    fprintf(events.value,"DRAIN_ACK,%llu,%llu\n",connection.Session(),T::Now()); events.Flush();
                    T::EvidenceFile report(dir+L"receiver-result.json");fprintf(report.value,"{\"frames\":%llu,\"au_bytes\":%llu,\"connections\":%llu,\"protocol_errors\":0,\"peak_payload\":%zu,\"drained\":true}\n",total,bytes,connections,peakPayload);report.Flush();return;
                }
            }
        } catch(const T::IoError& e) {
            fprintf(events.value,"DISCONNECTED_%d,%llu,%llu\n",e.code,connection.Session(),T::Now()); events.Flush(); state.Session(0);
        }
    }
    P::Require(state.stop,"live deadline without DRAIN");
}

int wmain(int argc,wchar_t** argv) {
    if(argc==2&&!wcscmp(argv[1],L"--self-test")) { try{Tests();return 0;}catch(const std::exception& e){fprintf(stderr,"TEST %s\n",e.what());return 1;} }
    if((argc==6||argc==7)&&!wcscmp(argv[1],L"--send-fixture")) {
        try {
            auto dir=PrivateDirectory(argv[2]);auto port=Number(argv[4]),count=Number(argv[5]);auto delay=argc==7?Number(argv[6]):35u;P::Require(port>=1024&&port<=65535&&count>=2&&count<=1700&&delay>=10&&delay<=100,"fixture bounds");
            T::Sender sender(uint16_t(port),dir);Sleep(1000);
            struct PaceTimer { HANDLE value=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);~PaceTimer(){if(value)CloseHandle(value);} } timer;
            P::Require(timer.value!=nullptr,"diagnostic high-resolution pacing timer");
            auto nextFixture=T::Now();
            std::ifstream file(std::filesystem::path(argv[3]),std::ios::binary);P::Require(bool(file),"fixture input");LARGE_INTEGER freq{};QueryPerformanceFrequency(&freq);
            for(unsigned i=0;i<count;++i){uint8_t head[12]{};file.read(reinterpret_cast<char*>(head),12);P::Require(file.gcount()==12,"fixture header");auto n=P::U32(head);P::Require(n&&n<=4*1024*1024,"fixture AU bound");
                std::vector<uint8_t> au(n);file.read(reinterpret_cast<char*>(au.data()),n);P::Require(file.gcount()==n,"fixture payload");LARGE_INTEGER qpc{};QueryPerformanceCounter(&qpc);
                sender.Consume({i+1,uint64_t(qpc.QuadPart),uint64_t(freq.QuadPart),P::U64(head+4),2400,1080,P::NalFlags(au.data(),n),n,au.data()});
                nextFixture+=uint64_t(delay)*1000000ULL;auto now=T::Now();if(nextFixture>now){LARGE_INTEGER due{};due.QuadPart=-LONGLONG((nextFixture-now)/100+1);P::Require(SetWaitableTimer(timer.value,&due,0,nullptr,nullptr,FALSE)!=0&&WaitForSingleObject(timer.value,1000)==WAIT_OBJECT_0,"fixture pacing");}else nextFixture=now;}
            sender.Finish();printf("PASS recorded-fixture transport; synthetic session times, NOT live Display 3\n");return 0;
        }catch(const std::exception& e){fprintf(stderr,"FIXTURE_ERROR %s\n",e.what());return 1;}
    }
    if(argc!=6){fprintf(stderr,"usage: Simulator --replay private-output fixture-AU-file count close-test(0/1) | --listen private-output port seconds capture-AUs(0/1)\n");return 2;}
    HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED); if(FAILED(co))return 1; HRESULT startup=MFStartup(MF_VERSION); if(FAILED(startup)){CoUninitialize();return 1;}
    int code=0;V::StageTrace uiTrace,networkTrace;V::stageTrace=&uiTrace;std::wstring traceDirectory;std::unique_ptr<V::ResourceWatch> observer;
    try {
        bool replay=!wcscmp(argv[1],L"--replay"); P::Require(replay||!wcscmp(argv[1],L"--listen"),"mode"); bool closeTest=replay&&Number(argv[5])==1; P::Require(Number(argv[5])<=1,"bounded mode flag");
        auto dir=PrivateDirectory(argv[2]);traceDirectory=dir;V::Stage("startup"); unsigned countOrSeconds=Number(argv[4]); P::Require(countOrSeconds>=1&&countOrSeconds<=300,"bounded run");
        unsigned port=replay?0:Number(argv[3]); P::Require(replay||(port>=1024&&port<=65535),"port");
        T::EvidenceFile events(dir+L"sessions.csv"),ledger(dir+L"rendered.csv"),resources(dir+L"resources.csv"),result(dir+L"visual-result.json"),samples(dir+L"sparse-samples.csv");
        fprintf(events.value,"event,session,time_ns\n"); fprintf(resources.value,"time_ns,private_bytes,working_set,handles,cpu_100ns\n");
        observer=std::make_unique<V::ResourceWatch>(dir);resourceWatch=observer.get();V::checkpoint=ResourceCheckpoint;
        V::GpuWindow gpu(&samples); Shared state; V::Decoder decoder(gpu,ledger,state.presentationGate,state.generation); auto began=T::Now(); auto lastResource=began;
        std::thread worker([&]{V::stageTrace=&networkTrace;try{if(replay)Replay(state,argv[3],countOrSeconds,events);else Receive(state,dir,port,countOrSeconds,Number(argv[5])==1,events);}catch(...){std::lock_guard<std::mutex> guard(state.lock);state.error=std::current_exception();}state.done=true;V::stageTrace=nullptr;});
        bool closed=false; uint64_t current=0; bool resized=false,minimized=false,restored=false;
        try {
            while(!state.done) {
                gpu.PollSparse();
                if(!gpu.Pump()){closed=true;state.stop=true;break;}
                V::Frame f; bool has=false,reset=false; uint64_t desired=0;
                {std::lock_guard<std::mutex> guard(state.lock);reset=state.reset;desired=state.desired;state.reset=false;has=state.queue.Pop(f);if(has){V::Identity(f.session,f.info.id,f.generation);V::Stage("queue_pop",state.queue.Size());}}
                if(reset){decoder.Reset(desired);current=desired;}
                if(has)decoder.Push(std::move(f)); else Sleep(1);
                if(state.drain&&!state.drained) { bool empty=false;{std::lock_guard<std::mutex> guard(state.lock);empty=state.queue.Size()==0;} if(empty){decoder.Drain();gpu.FinishSparse();state.drained=true;} }
                auto now=T::Now();
                if(closeTest&&now-began>2800000000ULL)PostMessageW(gpu.window,WM_CLOSE,0,0);
                if(replay && now-began>1000000000ULL && !resized){SetWindowPos(gpu.window,nullptr,0,0,1000,700,SWP_NOMOVE|SWP_NOZORDER|SWP_NOACTIVATE);resized=true;}
                if(replay && now-began>2000000000ULL && !minimized){ShowWindow(gpu.window,SW_MINIMIZE);minimized=true;}
                if(replay && now-began>2200000000ULL && !restored){ShowWindow(gpu.window,SW_SHOWNOACTIVATE);restored=true;}
                if(now-lastResource>=1000000000ULL) {
                    PROCESS_MEMORY_COUNTERS_EX pm{}; pm.cb=sizeof(pm); P::Require(GetProcessMemoryInfo(GetCurrentProcess(),reinterpret_cast<PROCESS_MEMORY_COUNTERS*>(&pm),sizeof(pm))!=0,"process memory");
                    DWORD handles=0; P::Require(GetProcessHandleCount(GetCurrentProcess(),&handles)!=0,"handle count"); FILETIME c{},e{},k{},u{}; P::Require(GetProcessTimes(GetCurrentProcess(),&c,&e,&k,&u)!=0,"CPU time");
                    uint64_t cpu=(uint64_t(k.dwHighDateTime)<<32)+k.dwLowDateTime+(uint64_t(u.dwHighDateTime)<<32)+u.dwLowDateTime;
                    fprintf(resources.value,"%llu,%zu,%zu,%lu,%llu\n",now,pm.PrivateUsage,pm.WorkingSetSize,handles,cpu);resources.Flush();lastResource=now;
                    wchar_t title[256]{}; swprintf_s(title,L"SweetDisplay | DXVA NV12 GPU | session %llu %s | decoded %llu presented %llu | drops %llu",current,decoder.lastPresentedSession==current&&current?L"LIVE":L"WAITING / prior content invalid",decoder.decoded,decoder.presented,decoder.renderDrops);SetWindowTextW(gpu.window,title);
                }
            }
            state.stop=true; worker.join(); if(state.error)std::rethrow_exception(state.error);
            if(!closed)decoder.Drain();else{state.Session(0);decoder.Reset(0);}
            gpu.FinishSparse();T::EvidenceFile sparseResult(dir+L"sparse-result.json");
            fprintf(sparseResult.value,"{\"mode\":\"async_64_pixel\",\"slot_bound\":1,\"submitted\":%llu,\"completed\":%llu,\"not_ready_polls\":%llu,\"busy_slot_skips\":%llu,\"pending\":0,\"blocking_map\":false}\n",gpu.sparseSubmitted,gpu.sparseCompleted,gpu.sparseBusy,gpu.sparseSkipped);sparseResult.Flush();
            if(!replay&&closed){T::EvidenceFile receiverResult(dir+L"receiver-result.json");fprintf(receiverResult.value,"{\"frames\":%llu,\"protocol_errors\":0,\"peak_payload\":%zu,\"drained\":false,\"closed_by_window\":true}\n",state.queue.received,state.peakPayload);receiverResult.Flush();}
            auto pending=state.queue.Size(); P::Require(state.queue.received==state.queue.admitted+state.queue.resync+state.queue.overflow,"received accounting");
            P::Require(state.queue.admitted==state.queue.taken+state.queue.resetDrops+pending,"queue accounting");
            P::Require(decoder.decoded==decoder.presented+decoder.renderDrops,"render accounting");
            P::Require(decoder.submitted==decoder.decoded+decoder.resetDrops,"decoder settled accounting");
            T::EvidenceFile gpuResult(dir+L"gpu-path.json");
            fprintf(gpuResult.value,"{\"hardware_device\":true,\"d3d11_aware\":true,\"manager_never_detached\":true,\"dxva_surface_proofs\":%llu,\"render_submissions\":%llu,\"presented\":%llu,\"minimized\":%llu,\"occluded\":%llu,\"present_busy\":%llu,\"stale_generation\":%llu,\"vendor_id\":%u,\"device_id\":%u,\"decoder_clsid\":\"62CE7E72-4C71-4D20-B15D-452831A87D9D\"}\n",gpu.surfaceProofs,gpu.renderSubmissions,decoder.presented,gpu.minimized,gpu.occluded,gpu.presentBusy,gpu.staleGeneration,gpu.adapter.VendorId,gpu.adapter.DeviceId);gpuResult.Flush();
            fprintf(result.value,"{\"outcome\":\"COMPONENT_MEASUREMENTS_ONLY\",\"replay\":%s,\"seconds\":%.6f,\"received\":%llu,\"admitted\":%llu,\"queue_taken\":%llu,\"queue_reset_drops\":%llu,\"queue_pending\":%zu,\"queue_overflow\":%llu,\"resync_skips\":%llu,\"encoded_queue_peak\":%zu,\"submitted\":%llu,\"decoded\":%llu,\"presented\":%llu,\"render_drops\":%llu,\"decoder_reset_drops\":%llu,\"decoder_pending_peak\":%zu,\"render_queue_bound\":1,\"decode_failures\":0,\"decoder_resets\":%llu,\"sparse_diagnostics\":%llu,\"resize_count\":%llu,\"closed_by_window\":%s,\"drained\":%s,\"hardware_surface_validation\":true,\"decoder\":\"Microsoft H264 CLSID_CMSH264DecoderMFT\",\"vendor_id\":%u,\"software_fallback\":false}\n",
                replay?"true":"false",double(T::Now()-began)/1e9,state.queue.received,state.queue.admitted,state.queue.taken,state.queue.resetDrops,pending,state.queue.overflow,state.queue.resync,state.queue.peak,
                decoder.submitted,decoder.decoded,decoder.presented,decoder.renderDrops,decoder.resetDrops,decoder.pendingPeak,decoder.resets,decoder.diagnostics,gpu.resizeCount,closed?"true":"false",state.drained?"true":"false",gpu.adapter.VendorId);result.Flush();
            fprintf(stdout,"PRESENT occluded=%llu busy=%llu minimized=%llu visible=%d\n",gpu.occluded,gpu.presentBusy,gpu.minimized,IsWindowVisible(gpu.window));
            P::Require(decoder.decoded&&decoder.presented,"actual decoded and presented frames");
        } catch(...) {state.stop=true;if(worker.joinable())worker.join();throw;}
        printf("PASS component run; PHASE3D live acceptance still requires external verification\n");
    } catch(const V::Failure& e){fprintf(stderr,"VISUAL_ERROR %s HRESULT=0x%08lX\n",e.what(),ULONG(e.hr));code=1;}
      catch(const std::exception& e){fprintf(stderr,"VISUAL_ERROR %s\n",e.what());code=1;}
    V::Stage("renderer_cleanup_complete");V::checkpoint=nullptr;if(observer)observer->Request(9);MFShutdown();CoUninitialize();V::Stage("shutdown");V::stageTrace=nullptr;
    if(observer){try{observer->Finish();}catch(const std::exception& e){fprintf(stderr,"RESOURCE_OBSERVER_ERROR %s\n",e.what());code=1;}}resourceWatch=nullptr;
    if(!traceDirectory.empty()){try{uiTrace.Save(traceDirectory+L"timing-ui.csv");networkTrace.Save(traceDirectory+L"timing-network.csv");}catch(const std::exception& e){fprintf(stderr,"TRACE_ERROR %s\n",e.what());code=1;}}return code;
}



