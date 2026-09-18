// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#define NOMINMAX
#include "../shared/FrameHandoffProtocol.h"
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <setupapi.h>
#include <sddl.h>
#include <mmsystem.h>
#include <atomic>
#include <cstdio>
#include <string>
#include <vector>
#include <stdexcept>
#include <memory>
#include "FirstFailDiagnostic.h"

using namespace SweetDisplay::Handoff;
using Microsoft::WRL::ComPtr;
static std::atomic<bool> stopping{false};
static BOOL WINAPI Signal(DWORD s){if(s==CTRL_C_EVENT||s==CTRL_BREAK_EVENT||s==CTRL_CLOSE_EVENT){stopping=true;return TRUE;}return FALSE;}
struct Error:std::runtime_error { DWORD code; Error(const char* text,DWORD c):std::runtime_error(text),code(c){} };
static void Hr(HRESULT h,const char* what){if(h!=S_OK)throw Error(what,DWORD(h));}
static uint64_t Qpc(){LARGE_INTEGER q{};QueryPerformanceCounter(&q);return q.QuadPart;}
struct Handle {
    HANDLE value=INVALID_HANDLE_VALUE;
    ~Handle(){Reset();} void Reset(){if(value&&value!=INVALID_HANDLE_VALUE)CloseHandle(value);value=INVALID_HANDLE_VALUE;}
    Handle()=default;Handle(const Handle&)=delete;Handle& operator=(const Handle&)=delete;
};
struct TimerResolution {TimerResolution(){timeBeginPeriod(1);}~TimerResolution(){timeEndPeriod(1);}};
struct FileClose {void operator()(FILE* f)const {if(f)fclose(f);}};
using File=std::unique_ptr<FILE,FileClose>;
static File Open(const std::wstring& path){FILE* f=nullptr;if(_wfopen_s(&f,path.c_str(),L"w"))throw Error("open evidence file",GetLastError());return File(f);}
#include "HardwareEncoder.h"
static void Discover(Handle& result){
    HDEVINFO info=SetupDiGetClassDevsW(&InterfaceId,nullptr,nullptr,DIGCF_PRESENT|DIGCF_DEVICEINTERFACE);
    if(info==INVALID_HANDLE_VALUE)throw Error("SetupDiGetClassDevs",GetLastError());
    std::wstring path;DWORD error=ERROR_NOT_FOUND;
    SP_DEVICE_INTERFACE_DATA data{};data.cbSize=sizeof(data);
    for(DWORD i=0;SetupDiEnumDeviceInterfaces(info,nullptr,&InterfaceId,i,&data);++i){
        DWORD size=0;SetupDiGetDeviceInterfaceDetailW(info,&data,nullptr,0,&size,nullptr);
        if(size<sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_W)){error=GetLastError();break;}
        std::vector<BYTE> storage(size);auto* detail=reinterpret_cast<SP_DEVICE_INTERFACE_DETAIL_DATA_W*>(storage.data());detail->cbSize=sizeof(*detail);
        if(!SetupDiGetDeviceInterfaceDetailW(info,&data,detail,size,nullptr,nullptr)){error=GetLastError();break;}
        if(!path.empty()){error=ERROR_DUP_NAME;path.clear();break;}path=detail->DevicePath;
    }
    SetupDiDestroyDeviceInfoList(info);
    if(path.empty())throw Error("discover unique SweetDisplay frame interface",error);
    result.value=CreateFileW(path.c_str(),GENERIC_READ|GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,nullptr,OPEN_EXISTING,0,nullptr);
    if(result.value==INVALID_HANDLE_VALUE)throw Error("open SweetDisplay interface (administrator required)",GetLastError());
}
static void Io(HANDLE h,ULONG code,const void* in,DWORD inSize,void* out,DWORD outSize){
    DWORD written=0;
    if(!DeviceIoControl(h,code,const_cast<void*>(in),inSize,out,outSize,&written,nullptr))throw Error("DeviceIoControl",GetLastError());
    if(written!=outSize)throw Error("unexpected reply size",ERROR_INVALID_DATA);
}
static State Read(HANDLE h,ULONG code=StateIo){State s{};Io(h,code,nullptr,0,&s,sizeof(s));
    if(!ValidPacket(s,sizeof(s))||!s.frequency||s.stats.depth+s.stats.held>Capacity)throw Error("invalid driver state",ERROR_INVALID_DATA);return s;}
struct Gpu {
    ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11Texture2D> textures[Capacity],sample;
    ComPtr<IDXGIKeyedMutex> mutexes[Capacity];Handle handles[Capacity];
    Connect config=Packet<Connect>();
    SweetDisplay::DesktopSample::Pixels samplePixels{};
    void Init(const State& s,bool sampling,bool encoding=false){
        ComPtr<IDXGIFactory4> factory;Hr(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"CreateDXGIFactory");
        ComPtr<IDXGIAdapter1> adapter;Hr(factory->EnumAdapterByLuid(s.adapter,IID_PPV_ARGS(&adapter)),"render adapter LUID");
        DXGI_ADAPTER_DESC1 ad{};Hr(adapter->GetDesc1(&ad),"GetDesc1");
        char name[256]{};WideCharToMultiByte(CP_UTF8,0,ad.Description,-1,name,sizeof(name),nullptr,nullptr);
        printf("SweetDisplayHost\nAdapter: SweetDisplay\nRender GPU: %s\nResolution: %ux%u\nFormat: BGRA8 (%u)\n",name,s.width,s.height,s.format);
        Hr(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT|(encoding?D3D11_CREATE_DEVICE_VIDEO_SUPPORT:0),nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context),"D3D11CreateDevice");
        config.epoch=s.epoch;config.adapter=s.adapter;config.width=s.width;config.height=s.height;config.format=s.format;
        GUID id{};Hr(CoCreateGuid(&id),"CoCreateGuid");
        wchar_t random[64]{};swprintf_s(random,L"%08x-%04x-%04x-%02x%02x%02x%02x%02x%02x%02x%02x",id.Data1,id.Data2,id.Data3,id.Data4[0],id.Data4[1],id.Data4[2],id.Data4[3],id.Data4[4],id.Data4[5],id.Data4[6],id.Data4[7]);
        PSECURITY_DESCRIPTOR raw=nullptr;
        if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(L"D:P(A;;GA;;;SY)(A;;GA;;;BA)(A;;GA;;;LS)(A;;GA;;;UD)",SDDL_REVISION_1,&raw,nullptr))throw Error("GPU security descriptor",GetLastError());
        std::unique_ptr<void,decltype(&LocalFree)> sd(raw,LocalFree);
        SECURITY_ATTRIBUTES sa{sizeof(sa),raw,FALSE};
        D3D11_TEXTURE2D_DESC desc{};desc.Width=s.width;desc.Height=s.height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT(s.format);desc.SampleDesc.Count=1;
        desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_SHADER_RESOURCE|D3D11_BIND_RENDER_TARGET;
        desc.MiscFlags=D3D11_RESOURCE_MISC_SHARED_NTHANDLE|D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX;
        for(uint32_t i=0;i<Capacity;++i){
            swprintf_s(config.names[i],L"Global\\SweetDisplay.%ls.%u",random,i);
            Hr(device->CreateTexture2D(&desc,nullptr,&textures[i]),"Create shared texture");
            ComPtr<IDXGIResource1> resource;Hr(textures[i].As(&resource),"IDXGIResource1");
            Hr(resource->CreateSharedHandle(&sa,DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,config.names[i],&handles[i].value),"CreateSharedHandle");
            Hr(textures[i].As(&mutexes[i]),"IDXGIKeyedMutex");
        }
        if(sampling){desc.Width=640;desc.Height=2;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
            Hr(device->CreateTexture2D(&desc,nullptr,&sample),"Create small sample texture");}
        if(!ValidConnect(config,sizeof(config)))throw Error("internal connection metadata validation",ERROR_INVALID_DATA);
    }
    void Sample(uint32_t slot,uint32_t& nonce,uint32_t& counter,uint64_t& hash){
        D3D11_BOX box{32,48,0,672,49,1};context->CopySubresourceRegion(sample.Get(),0,0,0,0,textures[slot].Get(),0,&box);
        box.top=96;box.bottom=97;context->CopySubresourceRegion(sample.Get(),0,0,1,0,textures[slot].Get(),0,&box);
        D3D11_MAPPED_SUBRESOURCE map{};Hr(context->Map(sample.Get(),0,D3D11_MAP_READ,0,&map),"Map small pattern sample");
        nonce=counter=0;hash=1469598103934665603ull;
        auto* bytes=static_cast<const BYTE*>(map.pData);
        for(uint32_t row=0;row<2;++row){
            memcpy(samplePixels.data()+row*640*4,bytes+row*map.RowPitch,640*4);
            uint32_t decoded=0;for(uint32_t bit=0;bit<32;++bit)if(bytes[row*map.RowPitch+(bit*20+8)*4]>200)decoded|=1u<<bit;
            if(row==0)nonce=decoded;else counter=decoded;
            for(uint32_t x=0;x<640*4;++x){hash^=bytes[row*map.RowPitch+x];hash*=1099511628211ull;}
        }
        context->Unmap(sample.Get(),0);
    }
};
struct Options {uint32_t seconds=35,slow=0,hold=0,sessions=1,nonce=0,inspect=0;bool sample=false,firstFail=false,classified=false,encode=false;SweetDisplay::Encoding::Settings encoding;std::wstring output;};
static bool Accounted(const State& s,uint64_t base){
    if(s.contention<base)return false;
    return s.stats.source==s.stats.delivered+s.stats.producerDrops+s.stats.hostQueueDrops+s.stats.busy+s.stats.invalid+s.stats.depth+s.stats.held+s.contention-base;
}
static void Run(const Options& o,uint32_t session){
    Handle file;
    // Retry only ordinary transient discovery/device readiness, with a deadline.
    uint64_t freq=0;LARGE_INTEGER q{};QueryPerformanceFrequency(&q);freq=q.QuadPart;
    uint64_t deadline=Qpc()+freq*15;State s{};
    for(;;){
        try{if(file.value==INVALID_HANDLE_VALUE)Discover(file);s=Read(file.value);if(s.active)break;}
        catch(const Error& e){if(e.code!=ERROR_NOT_FOUND&&e.code!=ERROR_DEVICE_NOT_CONNECTED&&e.code!=ERROR_NOT_READY)throw;file.Reset();}
        if(stopping||Qpc()>deadline)throw Error("source readiness timeout",WAIT_TIMEOUT);Sleep(250);
    }
    if(s.frequency!=freq||s.width!=2400||s.height!=1080||s.format!=87)throw Error("unexpected source geometry/clock",ERROR_INVALID_DATA);
    std::wstring prefix=o.output+L"/session-"+std::to_wstring(session);
    if(o.inspect){uint64_t first=s.totalSource,begin=Qpc();Sleep(o.inspect*1000);s=Read(file.value);const double elapsed=double(Qpc()-begin)/freq;
        auto f=Open(prefix+L"-absent.json");fprintf(f.get(),"{\"connected\":%u,\"source_delta\":%llu,\"active\":%u,\"no_host\":%llu,\"seconds\":%.9f,\"source_fps\":%.6f}\n",s.connected,s.totalSource-first,s.active,s.noHost,elapsed,(s.totalSource-first)/elapsed);
        if(s.connected||!s.active||s.totalSource<=first)throw Error("absent Host source did not advance",ERROR_INVALID_DATA);return;}
    Gpu gpu;gpu.Init(s,o.sample,o.encode);
    std::unique_ptr<SweetDisplay::Encoding::Encoder> encoder;
    if(o.encode)encoder=std::make_unique<SweetDisplay::Encoding::Encoder>(gpu.device.Get(),gpu.context.Get(),s.adapter,o.encoding,o.output,freq);
    SweetDisplay::FirstFail::Diagnostic diagnostic;
    if(o.firstFail){ID3D11Texture2D* textures[Capacity]{};for(uint32_t i=0;i<Capacity;++i)textures[i]=gpu.textures[i].Get();diagnostic.Init(o.output,o.nonce,gpu.config,textures,o.classified);}
    Io(file.value,ConnectIo,&gpu.config,sizeof(gpu.config),nullptr,0);
    printf("Frame source: CONNECTED; capacity=%u; session=%u\n",Capacity,session);
    auto csv=Open(prefix+L"-frames.csv");
    fprintf(csv.get(),"frame_id,source_qpc,receive_qpc,interval_ms,age_ms,width,height,format,flags,id_gap,nonce,counter,sample_hash,presentation_id,resource_acquired_qpc,sample_complete_qpc,mutex_wait_ms,epoch,slot,host_texture\n");
    auto telemetry=Open(prefix+L"-telemetry.csv");
    fprintf(telemetry.get(),"qpc,epoch,connected,source_frames,received,producer_drops,host_stale_drops,busy,invalid,contention,ready,held,high_water,last_id\n");
    uint64_t start=Qpc(),lastPrint=start,previousCount=0,received=0,firstId=0,lastId=0,firstQpc=0,lastQpc=0,minInterval=UINT64_MAX,maxInterval=0,gaps=0,changes=0,lastHash=0,matches=0;
    uint64_t contentionStart=s.contention;double ageTotal=0,maxAge=0;
    uint64_t lastPresentation=0;
    for(;!stopping && Qpc()-start<uint64_t(o.seconds)*freq;){
        if(encoder)encoder->Pump();
        s=Read(file.value,FetchIo);
        if(s.error)throw Error("driver GPU handoff error",DWORD(s.error));
        if(o.classified){
            if(s.epoch!=gpu.config.epoch||!s.connected||s.stats.highWater>Capacity||s.stats.invalid||s.stats.delivered!=received)throw Error("integrity: connection/queue/ack association",ERROR_INVALID_DATA);
            // source/contention atomics can momentarily lead the guarded queue snapshot.
            // Re-read only a mismatch; never edit or omit any recorded frame/drop.
            if(!Accounted(s,contentionStart)){bool resolved=false;for(int retry=0;retry<10&&!resolved;++retry){Sleep(1);auto settled=Read(file.value);resolved=Accounted(settled,contentionStart);}if(!resolved)throw Error("integrity: exact drop accounting",ERROR_INVALID_DATA);}
        }
        if(s.frame.id){
            auto& frame=s.frame;uint64_t receive=Qpc();
            if(!ValidFrame(frame,gpu.config.epoch,lastId,lastQpc,receive))throw Error("invalid/out-of-order frame",ERROR_INVALID_DATA);
            if(o.classified&&frame.presentation<=lastPresentation)throw Error("integrity: presentation ordering",ERROR_INVALID_DATA);
            HRESULT hr=S_OK;uint64_t waitStart=Qpc();
            do{hr=gpu.mutexes[frame.slot]->AcquireSync(1,0);if(hr==WAIT_TIMEOUT)Sleep(1);}while(hr==WAIT_TIMEOUT&&!stopping&&Qpc()-waitStart<freq/2);
            Hr(hr,"Host AcquireSync");
            const uint64_t resourceAcquired=Qpc();
            if(o.hold){if(!received){printf("TEST: GPU lease HELD; crash-cleanup probe enabled\n");fflush(stdout);}Sleep(o.hold);}
            uint32_t nonce=0,counter=0;uint64_t hash=0;
            try{if(o.sample)gpu.Sample(frame.slot,nonce,counter,hash);}
            catch(...){gpu.mutexes[frame.slot]->ReleaseSync(0);throw;}
            const uint64_t sampleComplete=o.sample?Qpc():0;
            uint32_t diagnosticFailure=0;
            if(o.firstFail){
                SweetDisplay::FirstFail::Record record{};record.state=s;record.receive=receive;record.acquired=resourceAcquired;record.sampled=sampleComplete;record.hash=hash;record.nonce=nonce;record.counter=counter;record.texture=uint64_t(gpu.textures[frame.slot].Get());
                diagnosticFailure=diagnostic.Observe(record,gpu.samplePixels);
                if(diagnosticFailure){try{diagnostic.Freeze(record,gpu.device.Get(),gpu.context.Get(),gpu.textures[frame.slot].Get());}catch(...){gpu.mutexes[frame.slot]->ReleaseSync(0);throw;}}
            }
            if(encoder&&!diagnosticFailure){try{encoder->Submit(gpu.textures[frame.slot].Get(),frame,nonce,counter);}catch(...){gpu.mutexes[frame.slot]->ReleaseSync(0);throw;}}
            Hr(gpu.mutexes[frame.slot]->ReleaseSync(0),"Host ReleaseSync");
            Ack ack=Packet<Ack>();ack.epoch=frame.epoch;ack.id=frame.id;ack.slot=frame.slot;
            Io(file.value,AckIo,&ack,sizeof(ack),nullptr,0);
            uint64_t gap=lastId?frame.id-lastId-1:0;gaps+=gap;
            double interval=lastQpc?Milliseconds(frame.qpc-lastQpc,freq):0,age=Milliseconds(receive-frame.qpc,freq);
            if(lastQpc){minInterval=(std::min)(minInterval,frame.qpc-lastQpc);maxInterval=(std::max)(maxInterval,frame.qpc-lastQpc);}
            if(!received){firstId=frame.id;firstQpc=frame.qpc;}
            if(received&&o.sample&&hash!=lastHash)++changes;
            if(o.sample&&nonce==o.nonce)++matches;
            fprintf(csv.get(),"%llu,%llu,%llu,%.6f,%.6f,%u,%u,%u,%u,%llu,%u,%u,%llu,%llu,%llu,%llu,%.6f,%llu,%u,%p\n",frame.id,frame.qpc,receive,interval,age,frame.width,frame.height,frame.format,frame.flags,gap,nonce,counter,hash,frame.presentation,resourceAcquired,sampleComplete,Milliseconds(resourceAcquired-waitStart,freq),frame.epoch,frame.slot,gpu.textures[frame.slot].Get());
            ++received;lastId=frame.id;lastQpc=frame.qpc;lastHash=hash;ageTotal+=age;maxAge=(std::max)(maxAge,age);
            lastPresentation=frame.presentation;
            if(diagnosticFailure){fflush(csv.get());Io(file.value,DisconnectIo,nullptr,0,nullptr,0);file.Reset();throw Error(o.classified?(diagnosticFailure==SweetDisplay::Content::D?"D: integrity failure; bounded evidence frozen":"E: UNCLASSIFIED content; bounded evidence frozen"):"first content-oracle failure; bounded evidence frozen",ERROR_INVALID_DATA);}
            if(o.slow)Sleep(o.slow);
        }else Sleep(2);
        auto now=Qpc();if(now-lastPrint>=freq){
            const uint64_t contentionDrops=s.contention-contentionStart;
            const uint64_t dropped=s.stats.producerDrops+s.stats.hostQueueDrops+s.stats.busy+s.stats.invalid+contentionDrops;
            fprintf(telemetry.get(),"%llu,%llu,%u,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%llu\n",now,s.epoch,s.connected,s.stats.source,received,s.stats.producerDrops,s.stats.hostQueueDrops,s.stats.busy,s.stats.invalid,contentionDrops,s.stats.depth,s.stats.held,s.stats.highWater,lastId);
            printf("Frames=%llu CurrentFPS=%.2f AverageFPS=%.2f Source=%llu DroppedTotal=%llu DroppedBeforeHost=%llu DroppedByHostQueue=%llu Busy=%llu Invalid=%llu Contention=%llu Queue=%u Held=%u HWM=%u LastID=%llu Age=%.2fms\n",received,double(received-previousCount)*freq/(now-lastPrint),double(received)*freq/(now-start),s.stats.source,dropped,s.stats.producerDrops,s.stats.hostQueueDrops,s.stats.busy,s.stats.invalid,contentionDrops,s.stats.depth,s.stats.held,s.stats.highWater,lastId,lastQpc?Milliseconds(now-lastQpc,freq):0);
            fflush(stdout);fflush(csv.get());fflush(telemetry.get());lastPrint=now;previousCount=received;
        }
    }
    uint64_t end=Qpc();s=Read(file.value);
    if(o.classified&&!Accounted(s,contentionStart)){for(int retry=0;retry<10&&!Accounted(s,contentionStart);++retry){Sleep(1);s=Read(file.value);}if(!Accounted(s,contentionStart))throw Error("integrity: final drop accounting",ERROR_INVALID_DATA);}
    Io(file.value,DisconnectIo,nullptr,0,nullptr,0);file.Reset();
    double seconds=double(end-start)/freq;
    if(encoder)encoder->Finish(seconds);
    auto report=Open(prefix+L"-result.json");
    fprintf(report.get(),"{\n\"seconds\":%.9f,\"frequency\":%llu,\"epoch\":%llu,\"source_frames\":%llu,\"received\":%llu,\"acknowledged\":%llu,\"fps\":%.6f,\"source_fps\":%.6f,\n\"width\":%u,\"height\":%u,\"format\":%u,\"first_id\":%llu,\"last_id\":%llu,\"id_gaps\":%llu,\"first_qpc\":%llu,\"last_qpc\":%llu,\n\"interval_avg_ms\":%.6f,\"interval_min_ms\":%.6f,\"interval_max_ms\":%.6f,\"age_avg_ms\":%.6f,\"age_max_ms\":%.6f,\n\"dropped_before_host\":%llu,\"dropped_host_queue\":%llu,\"busy\":%llu,\"invalid\":%llu,\"contention\":%llu,\"queue_depth_end\":%u,\"held_end\":%u,\"high_water\":%u,\n\"sample_changes\":%llu,\"nonce_matches\":%llu,\"clean_disconnect\":true\n}\n",
        seconds,freq,s.epoch,s.stats.source,received,s.stats.delivered,received/seconds,s.stats.source/seconds,s.width,s.height,s.format,firstId,lastId,gaps,firstQpc,lastQpc,
        received>1?Milliseconds(lastQpc-firstQpc,freq)/double(received-1):0,minInterval==UINT64_MAX?0:Milliseconds(minInterval,freq),Milliseconds(maxInterval,freq),received?ageTotal/received:0,maxAge,
        s.stats.producerDrops,s.stats.hostQueueDrops,s.stats.busy,s.stats.invalid,s.contention-contentionStart,s.stats.depth,s.stats.held,s.stats.highWater,changes,matches);
    printf("Frame source: DISCONNECTED; received=%llu elapsed=%.3fs FPS=%.3f\n",received,seconds,received/seconds);
    if(received<2||s.stats.invalid||s.stats.delivered!=received||(o.sample&&!o.classified&&(matches!=received||changes==0)))throw Error("integration validation failed; inspect report",ERROR_INVALID_DATA);
}
int wmain(int argc,wchar_t** argv){
    try{
        Options o;
        for(int i=1;i<argc;++i){std::wstring a=argv[i];
            if(a==L"--sample")o.sample=true;
            else if(a==L"--encode")o.encode=true;
            else if(a==L"--encode-uncapped")o.encoding.uncappedSubmission=true;
            else if(a==L"--first-fail"){o.firstFail=true;o.sample=true;}
            else if(a==L"--classified"){o.classified=true;o.firstFail=true;o.sample=true;}
            else if(i+1<argc){const wchar_t* v=argv[++i];if(a==L"--seconds")o.seconds=wcstoul(v,nullptr,10);else if(a==L"--output")o.output=v;
                else if(a==L"--encode-width")o.encoding.width=wcstoul(v,nullptr,10);else if(a==L"--encode-height")o.encoding.height=wcstoul(v,nullptr,10);else if(a==L"--encode-fps")o.encoding.fps=wcstoul(v,nullptr,10);else if(a==L"--encode-bitrate")o.encoding.bitrate=wcstoul(v,nullptr,10);
                else if(a==L"--slow-ms")o.slow=wcstoul(v,nullptr,10);else if(a==L"--hold-ms")o.hold=wcstoul(v,nullptr,10);else if(a==L"--sessions")o.sessions=wcstoul(v,nullptr,10);else if(a==L"--nonce")o.nonce=wcstoul(v,nullptr,16);else if(a==L"--inspect-seconds")o.inspect=wcstoul(v,nullptr,10);else throw Error("unknown option",ERROR_INVALID_PARAMETER);}
            else throw Error("missing option value",ERROR_INVALID_PARAMETER);
        }
        if(o.output.empty()||!o.seconds||o.seconds>86400||!o.sessions||o.sessions>10||o.slow>1000||o.hold>1000||o.inspect>60)throw Error("usage: --output existing-private-directory [--seconds 35 (max 86400) --sample --nonce HEX --sessions 2 --slow-ms 100 --hold-ms 1000 --inspect-seconds 3]",ERROR_INVALID_PARAMETER);
        if(o.firstFail&&(!o.nonce||o.sessions!=1||o.inspect))throw Error("first-fail requires a nonzero nonce and one streaming session",ERROR_INVALID_PARAMETER);
        if(o.encode&&(o.sessions!=1||o.inspect||o.encoding.width<800||o.encoding.width>2400||o.encoding.height<360||o.encoding.height>1080||(o.encoding.width%2)||(o.encoding.height%2)||!o.encoding.fps||o.encoding.fps>60||o.encoding.bitrate<100000||o.encoding.bitrate>100000000))throw Error("invalid bounded encoder settings",ERROR_INVALID_PARAMETER);
        if(o.encoding.uncappedSubmission&&!o.encode)throw Error("--encode-uncapped requires --encode",ERROR_INVALID_PARAMETER);
        SetConsoleCtrlHandler(Signal,TRUE);TimerResolution timer;
        uint32_t retries=0;
        for(uint32_t i=1;i<=o.sessions&&!stopping;){
            try {Run(o,i);++i;retries=0;if(i<=o.sessions)Sleep(1000);}
            catch(const Error& e){
                if(o.classified){auto f=Open(o.output+L"/diagnostic-error.json");fprintf(f.get(),"{\"code\":%lu,\"message\":\"%s\"}\n",e.code,e.what());throw;}
                // Device removal/source-epoch changes retire all local resources via RAII.
                // GPU access/validation failures remain fatal and retain their exact code.
                if((e.code!=ERROR_DEVICE_NOT_CONNECTED&&e.code!=ERROR_NOT_READY)||++retries>5)throw;
                printf("Frame source: DISCONNECTED; reconnect attempt=%u code=%lu\n",retries,e.code);fflush(stdout);Sleep(500);
            }
        }
        return 0;
    }catch(const Error& e){fprintf(stderr,"SweetDisplayHost ERROR: %s; code=%lu (0x%08lX)\n",e.what(),e.code,e.code);return 1;}
    catch(const std::exception& e){fprintf(stderr,"SweetDisplayHost ERROR: %s\n",e.what());return 1;}
}

