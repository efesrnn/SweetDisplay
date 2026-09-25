// SweetDisplay PHASE 3A: real D3D11 presentations on the active SWT0001 desktop.
#define NOMINMAX
#include <windows.h>
#include <d3d11_1.h>
#include <dxgi1_4.h>
#include <wrl.h>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <vector>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <share.h>
#include <cerrno>
#include "PatternObservation.h"
using Microsoft::WRL::ComPtr;
static void Check(ULONG code,const char* action) {if(code){char text[256];sprintf_s(text,"%s: code=%lu (0x%08lX)",action,code,code);throw std::runtime_error(text);}}
static void Hr(HRESULT hr,const char* action){if(hr!=S_OK)Check(ULONG(hr),action);}
static FILE* Open(const std::wstring& path,const wchar_t* mode){FILE* f=nullptr;const auto e=_wfopen_s(&f,path.c_str(),mode);if(e||!f)Check(e?e:ERROR_OPEN_FAILED,"Open output");return f;}
static FILE* OpenSharedRead(const std::wstring& path){FILE* f=_wfsopen(path.c_str(),L"w",_SH_DENYWR);if(!f)Check(errno?errno:ERROR_OPEN_FAILED,"Open shared output");return f;}
static uint64_t Qpc(){LARGE_INTEGER q{};QueryPerformanceCounter(&q);return q.QuadPart;}
static bool closed=false;
static SweetDisplay::PatternObservation* observation=nullptr;
static std::string observationError;
static FILE* touchLog=nullptr;static uint64_t touchRows=0;
static LRESULT CALLBACK WindowProc(HWND w,UINT m,WPARAM a,LPARAM b){
 if(observation){try{observation->Message(m,a,b);}catch(const std::exception& e){observationError=e.what();}}
 if(touchLog&&(m==WM_POINTERDOWN||m==WM_POINTERUPDATE||m==WM_POINTERUP)&&touchRows<10000){POINTER_INFO info{};if(GetPointerInfo(GET_POINTERID_WPARAM(a),&info)){POINT local=info.ptPixelLocation;ScreenToClient(w,&local);const char* event=m==WM_POINTERDOWN?"DOWN":m==WM_POINTERUP?"UP":"MOVE";const char* target="none";if(local.x<280&&local.y>100&&local.y<360)target="top-left";else if(local.x>2120&&local.y>100&&local.y<360)target="top-right";else if(local.x<280&&local.y>740)target="bottom-left";else if(local.x>2120&&local.y>740)target="bottom-right";else if(local.x>1060&&local.x<1340&&local.y>400&&local.y<680)target="center";fprintf(touchLog,"%llu,%s,%u,%ld,%ld,%ld,%ld,%s\n",++touchRows,event,GET_POINTERID_WPARAM(a),info.ptPixelLocation.x,info.ptPixelLocation.y,local.x,local.y,target);fflush(touchLog);}}
 if(m==WM_DESTROY){closed=true;PostQuitMessage(0);return 0;}
 if(m==WM_KEYDOWN&&a==VK_ESCAPE){DestroyWindow(w);return 0;}
 if(m==WM_PAINT){ValidateRect(w,nullptr);return 0;}
 return DefWindowProcW(w,m,a,b);
}
struct Display {
    DISPLAYCONFIG_PATH_INFO path{};
    DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};
    DISPLAYCONFIG_TARGET_DEVICE_NAME target{};
    DEVMODEW mode{};
};
static std::vector<Display> Displays() {
    UINT32 n=0,m=0;
    Check(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&n,&m),"GetDisplayConfigBufferSizes");
    std::vector<DISPLAYCONFIG_PATH_INFO> paths(n);
    std::vector<DISPLAYCONFIG_MODE_INFO> modes(m);
    Check(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&n,paths.data(),&m,modes.data(),nullptr),"QueryDisplayConfig");
    std::vector<Display> result;
    for (UINT32 i=0;i<n;++i) {
        Display d{}; d.path=paths[i];
        d.source.header={DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,sizeof(d.source),d.path.sourceInfo.adapterId,d.path.sourceInfo.id};
        d.target.header={DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,sizeof(d.target),d.path.targetInfo.adapterId,d.path.targetInfo.id};
        Check(DisplayConfigGetDeviceInfo(&d.source.header),"Get source name");
        Check(DisplayConfigGetDeviceInfo(&d.target.header),"Get target name");
        d.mode.dmSize=sizeof(d.mode);
        if (!EnumDisplaySettingsExW(d.source.viewGdiDeviceName,ENUM_CURRENT_SETTINGS,&d.mode,0))
            Check(GetLastError() ? GetLastError() : ERROR_NOT_FOUND,"EnumDisplaySettingsEx");
        result.push_back(d);
    }
    return result;
}
static Display Target() {
    std::vector<Display> candidates;
    for (const auto& d:Displays()) {
        std::wstring path=d.target.monitorDevicePath;
        std::transform(path.begin(),path.end(),path.begin(),[](wchar_t c){return static_cast<wchar_t>(towupper(c));});
        if (path.find(L"SWT0001")!=std::wstring::npos) candidates.push_back(d);
    }
    if (candidates.size()!=1) throw std::runtime_error("Expected one active SWT0001 display");
    const auto& d=candidates[0];
    if (d.path.targetInfo.outputTechnology!=DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED ||
        d.mode.dmPelsWidth!=2400 || d.mode.dmPelsHeight!=1080 ||
        d.path.targetInfo.refreshRate.Denominator==0 ||
        double(d.path.targetInfo.refreshRate.Numerator)/d.path.targetInfo.refreshRate.Denominator<59.9)
        throw std::runtime_error("SWT0001 is not an active indirect target at 2400x1080/60");
    return d;
}
static void Inventory(const std::wstring& file) {
    FILE* f=Open(file,L"w");
    for (const auto& d:Displays()) {
        fwprintf(f,L"source=%ls\nfriendly=%ls\nmonitor=%ls\nsource_adapter=%08lx:%08lx source_id=%u target_id=%u\nposition=%ld,%ld dimensions=%lux%lu refresh=%u/%u technology=%u\n\n",
            d.source.viewGdiDeviceName,d.target.monitorFriendlyDeviceName,d.target.monitorDevicePath,
            static_cast<ULONG>(d.path.sourceInfo.adapterId.HighPart),d.path.sourceInfo.adapterId.LowPart,
            d.path.sourceInfo.id,d.path.targetInfo.id,d.mode.dmPosition.x,d.mode.dmPosition.y,
            d.mode.dmPelsWidth,d.mode.dmPelsHeight,d.path.targetInfo.refreshRate.Numerator,
            d.path.targetInfo.refreshRate.Denominator,d.path.targetInfo.outputTechnology);
    }
    fclose(f);
}
int wmain(int argc,wchar_t** argv) {
    try {
        if(argc<4||argc>7)throw std::runtime_error("usage: SweetDisplayGpuPattern existing-evidence-directory nonce-hex seconds [render-fps] [sync-interval] [observe|cover|snapshot|minimize|foreground|touch]");
        const std::wstring dir=argv[1];const uint32_t nonce=wcstoul(argv[2],nullptr,16),seconds=wcstoul(argv[3],nullptr,10);
        const uint32_t renderFps=argc>4?wcstoul(argv[4],nullptr,10):60,syncInterval=argc>5?wcstoul(argv[5],nullptr,10):1;const bool touchMode=argc>6&&!wcscmp(argv[6],L"touch");
        if(renderFps>240||syncInterval>1)throw std::runtime_error("Invalid pacing configuration");
        if(!seconds||seconds>86400)throw std::runtime_error("Duration must be 1..86400 seconds");
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
        const auto target=Target();Inventory(dir+L"/pattern-display.txt");
        WNDCLASSW wc{};wc.hInstance=GetModuleHandleW(nullptr);wc.lpfnWndProc=WindowProc;wc.lpszClassName=L"SweetDisplayGpuPattern";wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
        if(!RegisterClassW(&wc))Check(GetLastError(),"RegisterClass");
        HWND window=CreateWindowExW(WS_EX_TOOLWINDOW|WS_EX_NOACTIVATE,wc.lpszClassName,L"SweetDisplay PHASE 3A GPU pattern",WS_POPUP,
            target.mode.dmPosition.x,target.mode.dmPosition.y,2400,1080,nullptr,nullptr,wc.hInstance,nullptr);
        if(!window)Check(GetLastError(),"CreateWindow");
        SweetDisplay::PatternObservation observations;observation=&observations;
        observations.Start(window,dir,nonce,touchMode?L"observe":argc>6?argv[6]:L"");
        std::unique_ptr<FILE,decltype(&fclose)> touchFile(nullptr,fclose);if(touchMode){touchFile.reset(OpenSharedRead(dir+L"/touch-target-events.csv"));touchLog=touchFile.get();fprintf(touchLog,"row,event,pointer_id,desktop_x,desktop_y,local_x,local_y,target\n");fflush(touchLog);}
        ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;ComPtr<ID3D11DeviceContext1> context1;
        Hr(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context),"D3D11CreateDevice");
        Hr(context.As(&context1),"D3D11 context1");D3D11_FEATURE_DATA_D3D11_OPTIONS options{};
        Hr(device->CheckFeatureSupport(D3D11_FEATURE_D3D11_OPTIONS,&options,sizeof(options)),"D3D11 options");
        if(!options.ClearView)throw std::runtime_error("GPU ClearView support required");
        ComPtr<IDXGIDevice> dxgi;ComPtr<IDXGIAdapter> adapter;ComPtr<IDXGIFactory2> factory;
        Hr(device.As(&dxgi),"DXGI device");Hr(dxgi->GetAdapter(&adapter),"DXGI adapter");Hr(adapter->GetParent(IID_PPV_ARGS(&factory)),"DXGI factory");
        DXGI_ADAPTER_DESC desc{};Hr(adapter->GetDesc(&desc),"Adapter description");
        char name[256]{};WideCharToMultiByte(CP_UTF8,0,desc.Description,-1,name,sizeof(name),nullptr,nullptr);
        DXGI_SWAP_CHAIN_DESC1 sc{};sc.Width=2400;sc.Height=1080;sc.Format=DXGI_FORMAT_B8G8R8A8_UNORM;sc.SampleDesc.Count=1;
        sc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT;sc.BufferCount=2;sc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_DISCARD;
        sc.Scaling=DXGI_SCALING_STRETCH;sc.AlphaMode=DXGI_ALPHA_MODE_IGNORE;sc.Flags=DXGI_SWAP_CHAIN_FLAG_FRAME_LATENCY_WAITABLE_OBJECT;
        ComPtr<IDXGISwapChain1> swap1;ComPtr<IDXGISwapChain3> swap;
        Hr(factory->CreateSwapChainForHwnd(device.Get(),window,&sc,nullptr,nullptr,&swap1),"CreateSwapChainForHwnd");Hr(swap1.As(&swap),"Swapchain3");
        Hr(factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER),"MakeWindowAssociation");
        Hr(swap->SetMaximumFrameLatency(1),"SetMaximumFrameLatency");
        HANDLE latency=swap->GetFrameLatencyWaitableObject();if(!latency)Check(GetLastError(),"GetFrameLatencyWaitableObject");
        struct CloseHandleOnExit {HANDLE h;~CloseHandleOnExit(){CloseHandle(h);}} closeLatency{latency};
        HANDLE pace=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_ALL_ACCESS);
        if(!pace)Check(GetLastError(),"Create high-resolution pacing timer");
        CloseHandleOnExit closePace{pace};
        // D3D11 rotates the resources behind buffer zero at Present; unlike D3D12,
        // the application does not index the physical back buffers itself.
        ComPtr<ID3D11RenderTargetView> renderView;
        {ComPtr<ID3D11Texture2D> buffer;Hr(swap->GetBuffer(0,IID_PPV_ARGS(&buffer)),"Swapchain buffer zero");Hr(device->CreateRenderTargetView(buffer.Get(),nullptr,&renderView),"Render target view");}
        using File=std::unique_ptr<FILE,decltype(&fclose)>;
        File log(Open(dir+L"/pattern.csv",L"w"),fclose);fprintf(log.get(),"paint_tick,qpc,present_return_qpc\n");
        if(!SetWindowPos(window,HWND_TOPMOST,target.mode.dmPosition.x,target.mode.dmPosition.y,2400,1080,SWP_NOACTIVATE|SWP_SHOWWINDOW))Check(GetLastError(),"Place SWT0001 window");
        printf("GPU pattern: %s; 2400x1080 BGRA8; flip-discard; latency=1; render limit=%u; Present(%u,0)\n",name,renderFps,syncInterval);fflush(stdout);
        LARGE_INTEGER fq{};QueryPerformanceFrequency(&fq);const uint64_t frequency=fq.QuadPart,start=Qpc();
        uint64_t lastReport=start,first=0,last=0,nextTick=0;uint32_t counter=0;
        const float black[4]={0,0,0,1},white[4]={1,1,1,1},marker[4]={1,0,0.86f,1};
        while(!closed&&Qpc()-start<uint64_t(seconds)*frequency){
            const DWORD wait=MsgWaitForMultipleObjects(1,&latency,FALSE,1000,QS_ALLINPUT);
            MSG msg{};while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){closed=true;break;}TranslateMessage(&msg);DispatchMessageW(&msg);}
            if(!observationError.empty())throw std::runtime_error(observationError);
            if(closed)break;
            if(observations.enabled&&IsIconic(window))continue;
            if(wait==WAIT_OBJECT_0+1)continue;
            if(wait!=WAIT_OBJECT_0)Check(wait,"Frame latency wait");
            // The DXGI latency event alone can follow a different physical
            // output cadence on a multi-monitor desktop. Bound new presentations
            // to 60 Hz using absolute QPC deadlines; skip missed deadlines.
            const uint64_t due=renderFps?start+nextTick*frequency/renderFps:0;
            while(!closed&&Qpc()<due){
                const auto remaining=due-Qpc();
                if(remaining>frequency)break; // QPC crossed due between reads.
                LARGE_INTEGER relative{};relative.QuadPart=-LONGLONG((remaining*10000000+frequency-1)/frequency);
                if(!SetWaitableTimer(pace,&relative,0,nullptr,nullptr,FALSE))Check(GetLastError(),"Arm pacing timer");
                const DWORD paced=MsgWaitForMultipleObjects(1,&pace,FALSE,1000,QS_ALLINPUT);
                if(paced!=WAIT_OBJECT_0&&paced!=WAIT_OBJECT_0+1)Check(paced,"Pacing timer wait");
                while(PeekMessageW(&msg,nullptr,0,0,PM_REMOVE)){if(msg.message==WM_QUIT){closed=true;break;}TranslateMessage(&msg);DispatchMessageW(&msg);}
            }
            if(closed)break;
            const uint64_t renderQpc=Qpc();++counter;
            if(observations.enabled)observations.ledger.Publish(counter,renderQpc);
            if(renderFps)nextTick=(std::max)(nextTick+1,(renderQpc-start)*renderFps/frequency+1);
            auto* view=renderView.Get();
            const float background[4]={0.05f,0.10f,float(counter%120)/240.0f+0.1f,1};context->ClearRenderTargetView(view,background);
            D3D11_RECT cells[64];std::vector<D3D11_RECT> on;on.reserve(64);
            for(int bit=0;bit<32;++bit){cells[bit]={32+bit*20,32,48+bit*20,64};cells[bit+32]={32+bit*20,80,48+bit*20,112};
                if(nonce&(1u<<bit))on.push_back(cells[bit]);if(counter&(1u<<bit))on.push_back(cells[bit+32]);}
            context1->ClearView(view,black,cells,64);if(!on.empty())context1->ClearView(view,white,on.data(),UINT(on.size()));
            const LONG x=LONG((uint64_t(counter)*13)%2100);D3D11_RECT moving{x,500,x+260,760};context1->ClearView(view,marker,&moving,1);
            if(touchMode){const D3D11_RECT targets[5]={{40,140,240,340},{2160,140,2360,340},{40,760,240,1040},{2160,760,2360,1040},{1110,450,1290,630}};const float colors[5][4]={{1,0.15f,0.15f,1},{0.15f,1,0.15f,1},{0.15f,0.35f,1,1},{1,0.75f,0.1f,1},{1,1,1,1}};for(unsigned i=0;i<5;++i)context1->ClearView(view,colors[i],&targets[i],1);}
            Hr(swap->Present(syncInterval,0),"Present");const uint64_t done=Qpc();
            if(!first)first=renderQpc;last=renderQpc;
            fprintf(log.get(),"%u,%llu,%llu\n",counter,renderQpc,done);
            if(done-lastReport>=frequency){if(observations.enabled)observations.Occlusion(swap->Present(0,DXGI_PRESENT_TEST));printf("Presented=%u RenderCadence=%.3f FPS\n",counter,first==last?0:double(counter-1)*frequency/(last-first));fflush(stdout);fflush(log.get());lastReport=done;}
        }
        File report(Open(dir+L"/pattern-result.json",L"w"),fclose);
        fprintf(report.get(),"{\"presented\":%u,\"frequency\":%llu,\"first_qpc\":%llu,\"last_qpc\":%llu,\"render_fps\":%.6f,\"render_limit\":%u,\"width\":2400,\"height\":1080,\"sync_interval\":%u,\"maximum_latency\":1}\n",counter,frequency,first,last,first==last?0:double(counter-1)*frequency/(last-first),renderFps,syncInterval);
        if(!closed)DestroyWindow(window);observation=nullptr;touchLog=nullptr;return 0;
    }catch(const std::exception& e){fprintf(stderr,"GPU pattern ERROR: %s\n",e.what());return 1;}
}

