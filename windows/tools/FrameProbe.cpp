// PHASE 1 only: ETW telemetry, one diagnostic frame, and a moving test window.
// This is not SweetDisplayHost and does not implement a continuous frame transport.
#define NOMINMAX
#include <windows.h>
#include <evntrace.h>
#include <evntcons.h>
#include <dwmapi.h>
#include <cstdio>
#include <cstdint>
#include <cwchar>
#include <vector>
#include <string>
#include <thread>
#include <atomic>
#include <algorithm>
#include <stdexcept>
#include "../driver/diagnostics/FrameEvents.h"
#pragma comment(lib,"advapi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"dwmapi.lib")
using namespace SweetDisplay::Diagnostic;

static void Check(ULONG result,const char* command) {
    if (result) { char text[256]; sprintf_s(text,"%s: error=%lu (0x%08lX)",command,result,result); throw std::runtime_error(text); }
}
static std::wstring Path(const std::wstring& root,const wchar_t* file) { return root+L"\\"+file; }
static FILE* Open(const std::wstring& path,const wchar_t* mode) {
    FILE* f=nullptr; const errno_t e=_wfopen_s(&f,path.c_str(),mode);
    if (e || !f) throw std::runtime_error("File open failed"); return f;
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
static uint32_t patternNonce=0,patternTick=0;
static FILE* patternLog=nullptr;
static std::wstring patternGdi;
static void RectColor(HDC dc,int x,int y,int w,int h,COLORREF color) {
    RECT r{x,y,x+w,y+h}; HBRUSH brush=CreateSolidBrush(color);
    FillRect(dc,&r,brush); DeleteObject(brush);
}
static void DrawPattern(HWND window,HDC dc) {
    RECT r{}; GetClientRect(window,&r);
    RectColor(dc,0,0,r.right,r.bottom,RGB(18,24,40));
    const COLORREF colors[]={RGB(230,30,45),RGB(25,200,90),RGB(30,90,235),RGB(240,210,25)};
    for (int i=0;i<4;++i) RectColor(dc,i*r.right/4,300,r.right/4,r.bottom-300,colors[i]);
    // Exact 32-bit nonce and paint counter in fixed-color cells, independent of text rasterization.
    for (int bit=0;bit<32;++bit) {
        RectColor(dc,32+bit*20,32,16,32,(patternNonce&(1u<<bit))?RGB(255,255,255):RGB(0,0,0));
        RectColor(dc,32+bit*20,80,16,32,(patternTick&(1u<<bit))?RGB(255,255,255):RGB(0,0,0));
    }
    RectColor(dc,static_cast<int>((patternTick*13)%2100),500,260,260,RGB(255,0,220));
    SetBkMode(dc,TRANSPARENT); SetTextColor(dc,RGB(255,255,255));
    HFONT font=CreateFontW(56,0,0,0,FW_BOLD,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,
        CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HGDIOBJ previous=SelectObject(dc,font);
    wchar_t label[256];
    swprintf_s(label,L"SweetDisplay PHASE 1 | %ls | nonce %08X | tick %u",patternGdi.c_str(),patternNonce,patternTick);
    TextOutW(dc,32,150,label,static_cast<int>(wcslen(label)));
    SelectObject(dc,previous); DeleteObject(font);
}
static LRESULT CALLBACK PatternProc(HWND w,UINT msg,WPARAM a,LPARAM b) {
    if (msg==WM_TIMER) {
        ++patternTick; LARGE_INTEGER q{}; QueryPerformanceCounter(&q);
        fprintf(patternLog,"%u,%llu\n",patternTick,static_cast<unsigned long long>(q.QuadPart)); fflush(patternLog);
        InvalidateRect(w,nullptr,FALSE); UpdateWindow(w); DwmFlush(); return 0;
    }
    if (msg==WM_PAINT) {
        PAINTSTRUCT ps{}; HDC dc=BeginPaint(w,&ps);
        RECT r{}; GetClientRect(w,&r);
        HDC mem=CreateCompatibleDC(dc); HBITMAP bitmap=CreateCompatibleBitmap(dc,r.right,r.bottom);
        HGDIOBJ previous=SelectObject(mem,bitmap); DrawPattern(w,mem);
        BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);
        SelectObject(mem,previous); DeleteObject(bitmap); DeleteDC(mem); EndPaint(w,&ps); return 0;
    }
    if (msg==WM_KEYDOWN && a==VK_ESCAPE) { DestroyWindow(w); return 0; }
    if (msg==WM_DESTROY) { KillTimer(w,1); PostQuitMessage(0); return 0; }
    return DefWindowProcW(w,msg,a,b);
}
static int Pattern(const std::wstring& dir,uint32_t nonce) {
    SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
    const auto d=Target();
    Inventory(Path(dir,L"pattern-display.txt"));
    patternGdi=d.source.viewGdiDeviceName; patternNonce=nonce; patternLog=Open(Path(dir,L"pattern.csv"),L"w");
    fprintf(patternLog,"paint_tick,qpc\n");
    WNDCLASSW wc{}; wc.lpfnWndProc=PatternProc; wc.hInstance=GetModuleHandleW(nullptr);
    wc.lpszClassName=L"SweetDisplayPhase1Pattern"; wc.hCursor=LoadCursor(nullptr,IDC_ARROW);
    if (!RegisterClassW(&wc)) Check(GetLastError(),"RegisterClass");
    HWND w=CreateWindowExW(WS_EX_TOOLWINDOW,wc.lpszClassName,L"SweetDisplay PHASE 1 changing pattern",
        WS_POPUP,100,100,800,360,nullptr,nullptr,wc.hInstance,nullptr);
    if (!w) Check(GetLastError(),"CreateWindowEx");
    ShowWindow(w,SW_SHOWNOACTIVATE);
    // Move this real, visibly updating window onto the exact queried indirect target.
    if (!SetWindowPos(w,HWND_TOPMOST,d.mode.dmPosition.x,d.mode.dmPosition.y,2400,1080,SWP_NOACTIVATE|SWP_SHOWWINDOW))
        Check(GetLastError(),"SetWindowPos DISPLAY3");
    if (!SetTimer(w,1,16,nullptr)) Check(GetLastError(),"SetTimer");
    MSG msg{}; int status;
    while ((status=GetMessageW(&msg,nullptr,0,0))>0) { TranslateMessage(&msg); DispatchMessageW(&msg); }
    fclose(patternLog); return status<0 ? 1 : 0;
}

struct Collector {
    FILE* csv=nullptr;
    Frame dump{};
    std::vector<unsigned char> pixels;
    uint32_t nextOffset=0,nonce=0,tick=0,pid=0;
    uint64_t frames=0,firstQpc=0,lastQpc=0,lastId=0,firstPresentation=0,lastPresentation=0;
    uint64_t minInterval=UINT64_MAX,maxInterval=0,frequency=0;
    bool completed=false,metadataOnly=false;
    std::string error;
    std::wstring dir;
    void Consume(EVENT_RECORD* record) {
        if (!error.empty() || !IsEqualGUID(record->EventHeader.ProviderId,Provider)) return;
        if (record->EventHeader.EventDescriptor.Version!=Version) { error="Wrong ETW schema version"; return; }
        const auto* data=static_cast<const unsigned char*>(record->UserData);
        const size_t bytes=record->UserDataLength;
        const auto id=record->EventHeader.EventDescriptor.Id;
        if(metadataOnly&&(id==BeginEvent||id==ChunkEvent||id==EndEvent)){error="Capture event in metadata-only session";return;}
        if (id==ErrorEvent && bytes==sizeof(Error)) {
            Error e{}; memcpy(&e,data,sizeof(e)); char text[160];
            sprintf_s(text,"Driver diagnostic stage=%u HRESULT=0x%08X frame=%llu",e.stage,
                static_cast<uint32_t>(e.code),static_cast<unsigned long long>(e.frameId)); error=text; return;
        }
        if (id==FrameEvent || id==BeginEvent) {
            if (bytes!=sizeof(Frame)) { error="Frame record size mismatch"; return; }
            Frame f{}; memcpy(&f,data,sizeof(f));
            if (f.magic!=Magic || f.version!=Version || f.size!=sizeof(Frame) || !f.qpcFrequency) { error="Frame header invalid"; return; }
            if (id==FrameEvent) {
                if(metadataOnly&&(f.width!=2400||f.height!=1080||f.format!=87)){error="Unexpected metadata-only frame geometry";return;}
                if (frames && (f.frameId<=lastId || f.acquiredQpc<=lastQpc)) { error="Frame ID/timestamp regression"; return; }
                if (frames && record->EventHeader.ProcessId!=pid) { error="Multiple producer processes"; return; }
                if (!frames) { firstQpc=f.acquiredQpc; firstPresentation=f.presentationFrameNumber; pid=record->EventHeader.ProcessId; }
                ++frames; lastQpc=f.acquiredQpc; lastId=f.frameId; lastPresentation=f.presentationFrameNumber; frequency=f.qpcFrequency;
                if (f.intervalQpc) { minInterval=std::min(minInterval,f.intervalQpc); maxInterval=std::max(maxInterval,f.intervalQpc); }
                fprintf(csv,"%llu,%llu,%llu,%llu,%u,%u,%u,%.6f,%u,%lu\n",
                    static_cast<unsigned long long>(f.frameId),static_cast<unsigned long long>(f.acquiredQpc),
                    static_cast<unsigned long long>(f.qpcFrequency),static_cast<unsigned long long>(f.presentQpc),
                    f.width,f.height,f.format,1000.0*f.intervalQpc/f.qpcFrequency,f.presentationFrameNumber,record->EventHeader.ProcessId);
            } else {
                if (!pixels.empty() || completed) { error="More than one diagnostic frame"; return; }
                const uint64_t size=uint64_t(f.width)*f.height*4;
                if (!size || size>MaxBytes) { error="Diagnostic image size invalid"; return; }
                dump=f; pixels.resize(static_cast<size_t>(size)); nextOffset=0;
            }
        } else if (id==ChunkEvent) {
            if (bytes<sizeof(Chunk)) { error="Short chunk"; return; }
            Chunk chunk{}; memcpy(&chunk,data,sizeof(chunk));
            if (chunk.frameId!=dump.frameId || chunk.offset!=nextOffset || chunk.bytes!=bytes-sizeof(Chunk) ||
                uint64_t(chunk.offset)+chunk.bytes>pixels.size()) { error="Missing/out-of-order/oversized chunk"; return; }
            memcpy(pixels.data()+chunk.offset,data+sizeof(Chunk),chunk.bytes); nextOffset+=chunk.bytes;
        } else if (id==EndEvent) {
            End end{}; if (bytes!=sizeof(end)) { error="Wrong end size"; return; }
            memcpy(&end,data,sizeof(end));
            if (end.frameId!=dump.frameId || end.bytes!=pixels.size() || nextOffset!=pixels.size() ||
                end.rowBytes!=dump.width*4 || end.hash!=Hash(pixels.data(),pixels.size())) { error="Incomplete or corrupt diagnostic image"; return; }
            if (dump.width!=2400 || dump.height!=1080) { error="Diagnostic image is not 2400x1080"; return; }
            uint32_t decoded=0;
            for (int row=0;row<2;++row) {
                uint32_t value=0;
                for (int bit=0;bit<32;++bit) {
                    const size_t offset=(size_t(row?96:48)*dump.width+40+bit*20)*4;
                    const auto* p=pixels.data()+offset;
                    if (p[0]>240 && p[1]>240 && p[2]>240) value|=(1u<<bit);
                    else if (p[0]>16 || p[1]>16 || p[2]>16) { error="Pattern cell is neither black nor white"; return; }
                }
                if (!row) decoded=value; else tick=value;
            }
            if (decoded!=nonce || !tick) { error="DISPLAY3 pattern nonce/counter does not match"; return; }
            if (dump.format==28) for(size_t i=0;i<pixels.size();i+=4) std::swap(pixels[i],pixels[i+2]);
            else if (dump.format!=87 && dump.format!=88) { error="Unsupported BMP input format"; return; }
            // CREATE_NEW guarantees the tool cannot overwrite or generate a second frame.
            const auto name=Path(dir,L"diagnostic-frame.bmp");
            HANDLE file=CreateFileW(name.c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
            if (file==INVALID_HANDLE_VALUE) { error="Cannot create unique diagnostic-frame.bmp"; return; }
            BITMAPFILEHEADER header{}; header.bfType=0x4d42;
            header.bfOffBits=sizeof(header)+sizeof(BITMAPINFOHEADER);
            header.bfSize=header.bfOffBits+static_cast<DWORD>(pixels.size());
            BITMAPINFOHEADER info{}; info.biSize=sizeof(info); info.biWidth=static_cast<LONG>(dump.width);
            info.biHeight=-static_cast<LONG>(dump.height); info.biPlanes=1; info.biBitCount=32; info.biCompression=BI_RGB;
            DWORD written=0;
            bool ok=WriteFile(file,&header,sizeof(header),&written,nullptr) && written==sizeof(header);
            ok=ok && WriteFile(file,&info,sizeof(info),&written,nullptr) && written==sizeof(info);
            ok=ok && WriteFile(file,pixels.data(),static_cast<DWORD>(pixels.size()),&written,nullptr) && written==pixels.size();
            CloseHandle(file); if (!ok) { error="Diagnostic frame write failed"; return; }
            completed=true;
        }
    }
};
static void WINAPI EventCallback(EVENT_RECORD* record) {
    auto* c=static_cast<Collector*>(record->UserContext);
    try { c->Consume(record); } catch (...) { c->error="Consumer exception"; }
}
static int Capture(const std::wstring& dir,uint32_t nonce,unsigned seconds,bool metadataOnly=false) {
    if (!seconds || seconds>(metadataOnly?300u:60u)) throw std::runtime_error("Invalid trace duration");
    const auto target=Target(); (void)target;
    Inventory(Path(dir,L"capture-display.txt"));
    if (GetFileAttributesW(Path(dir,L"diagnostic-frame.bmp").c_str())!=INVALID_FILE_ATTRIBUTES)
        throw std::runtime_error("One diagnostic frame already exists; refusing another capture");
    Collector c{}; c.dir=dir; c.nonce=nonce;c.metadataOnly=metadataOnly;
    c.csv=Open(Path(dir,L"frames.csv"),L"w");
    fprintf(c.csv,"frame_id,acquired_qpc,qpc_frequency,present_qpc,width,height,dxgi_format,interval_ms,presentation_frame_number,producer_pid\n");
    std::wstring sessionName=(metadataOnly?L"SweetDisplay.Metadata.":L"SweetDisplay.Phase1.")+std::to_wstring(GetCurrentProcessId());
    std::vector<unsigned char> buffer(sizeof(EVENT_TRACE_PROPERTIES)+(sessionName.size()+1)*sizeof(wchar_t));
    auto* props=reinterpret_cast<EVENT_TRACE_PROPERTIES*>(buffer.data());
    props->Wnode.BufferSize=static_cast<ULONG>(buffer.size()); props->Wnode.Flags=WNODE_FLAG_TRACED_GUID;
    props->Wnode.ClientContext=1; props->BufferSize=64; props->MinimumBuffers=metadataOnly?16:256; props->MaximumBuffers=metadataOnly?32:512;
    props->LogFileMode=EVENT_TRACE_REAL_TIME_MODE; props->FlushTimer=1; props->LoggerNameOffset=sizeof(EVENT_TRACE_PROPERTIES);
    TRACEHANDLE session=0;
    ULONG code=StartTraceW(&session,sessionName.c_str(),props);
    if (code) { fclose(c.csv); Check(code,"StartTrace"); }
    EVENT_TRACE_LOGFILEW logfile{}; logfile.LoggerName=sessionName.data();
    logfile.ProcessTraceMode=PROCESS_TRACE_MODE_REAL_TIME|PROCESS_TRACE_MODE_EVENT_RECORD;
    logfile.EventRecordCallback=EventCallback; logfile.Context=&c;
    TRACEHANDLE consumer=OpenTraceW(&logfile);
    if (consumer==INVALID_PROCESSTRACE_HANDLE) {
        code=GetLastError(); ControlTraceW(session,sessionName.c_str(),props,EVENT_TRACE_CONTROL_STOP);
        fclose(c.csv); Check(code,"OpenTrace");
    }
    std::atomic<ULONG> traceCode{ERROR_SUCCESS};
    std::thread processing([&]{traceCode=ProcessTrace(&consumer,1,nullptr,nullptr);});
    code=EnableTraceEx2(session,&Provider,EVENT_CONTROL_CODE_ENABLE_PROVIDER,4,metadataOnly?MetadataKeyword:MetadataKeyword|CaptureKeyword,0,0,nullptr);
    if (!code) Sleep(seconds*1000);
    const ULONG stop=ControlTraceW(session,sessionName.c_str(),props,EVENT_TRACE_CONTROL_STOP);
    CloseTrace(consumer); processing.join(); fclose(c.csv);
    const double elapsed=c.frequency ? double(c.lastQpc-c.firstQpc)/c.frequency : 0;
    const double fps=elapsed>0 ? (c.frames-1)/elapsed : 0;
    FILE* summary=Open(Path(dir,L"result.json"),L"w");
    const bool pass=!code && !stop && (traceCode==0 || traceCode==ERROR_CANCELLED) &&
        c.error.empty() && (metadataOnly?!c.completed:c.completed) && c.frames>=120 && c.lastPresentation>c.firstPresentation &&
        props->EventsLost==0 && props->RealTimeBuffersLost==0 && (metadataOnly||(c.dump.width==2400 && c.dump.height==1080));
    fprintf(summary,"{\n\"metadata_only\":%s,\"status\":\"%s\",\"frames\":%llu,\"fps\":%.6f,\"elapsed_seconds\":%.6f,"
        "\"dump_frame_id\":%llu,\"dump_paint_tick\":%u,\"nonce\":%u,\"width\":%u,\"height\":%u,\"dxgi_format\":%u,"
        "\"producer_pid\":%u,\"events_lost\":%lu,\"buffers_lost\":%lu,"
        "\"enable_code\":%lu,\"stop_code\":%lu,\"process_trace_code\":%lu,\"error\":\"%s\"\n}\n",
        metadataOnly?"true":"false",pass?"VERIFIED":"NOT YET TESTED",static_cast<unsigned long long>(c.frames),fps,elapsed,
        static_cast<unsigned long long>(c.dump.frameId),c.tick,nonce,c.dump.width,c.dump.height,c.dump.format,c.pid,
        props->EventsLost,props->RealTimeBuffersLost,code,stop,static_cast<ULONG>(traceCode),c.error.c_str());
    fclose(summary);
    printf("frames=%llu fps=%.3f dump=%s events_lost=%lu buffers_lost=%lu error=%s\n",
        static_cast<unsigned long long>(c.frames),fps,metadataOnly?"disabled":c.completed?"one verified pattern":"missing",
        props->EventsLost,props->RealTimeBuffersLost,c.error.c_str());
    return pass?0:1;
}
int wmain(int argc,wchar_t** argv) {
    try {
        if (argc==3 && wcscmp(argv[1],L"--inventory")==0) { Inventory(argv[2]); return 0; }
        if (argc==4 && wcscmp(argv[1],L"--pattern")==0) return Pattern(argv[2],wcstoul(argv[3],nullptr,16));
        if (argc==5 && wcscmp(argv[1],L"--capture")==0)
            return Capture(argv[2],wcstoul(argv[3],nullptr,16),wcstoul(argv[4],nullptr,10));
        if (argc==4 && wcscmp(argv[1],L"--metadata")==0)
            return Capture(argv[2],0,wcstoul(argv[3],nullptr,10),true);
        fprintf(stderr,"Usage: --inventory file | --pattern evidenceDir hexNonce | --capture evidenceDir hexNonce seconds | --metadata evidenceDir seconds\n");
        return 2;
    } catch (const std::exception& e) { fprintf(stderr,"%s\n",e.what()); return 1; }
}
