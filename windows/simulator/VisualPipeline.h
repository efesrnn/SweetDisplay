// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include "../protocol/TransportSender.h"
#include "StageTrace.h"
#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <codecapi.h>
#include <d3d11.h>
#include <d3d10.h>
#include <dxgi1_2.h>
#include <wrl.h>
#include <deque>
#include <map>
#include <mutex>

namespace SweetDisplay::Visual {
namespace P = Protocol;
namespace T = Transport;
using Microsoft::WRL::ComPtr;
// Optional component diagnostics. No hook is installed in the protected v1 binary.
inline void (*checkpoint)(const char*)=nullptr;
inline void Checkpoint(const char* stage){Stage(stage);if(checkpoint)checkpoint(stage);}
struct Failure : std::runtime_error {
    HRESULT hr;
    Failure(const char* text, HRESULT code) : std::runtime_error(text), hr(code) {}
};
inline void Check(HRESULT hr, const char* text) { if (FAILED(hr)) throw Failure(text, hr); }
struct Frame {
    uint64_t session=0, sequence=0, received=0, generation=0;
    P::FrameInfo info{};
    std::vector<uint8_t> au;
};
// ACK continues to mean transport receipt, not presentation. Lost compressed
// dependencies invalidate all queued references; restart only at IDR+SPS+PPS.
class Queue {
    std::deque<Frame> frames;
    uint64_t session=0;
    bool recovery=true;
public:
    static constexpr size_t Bound=3;
    uint64_t received=0, admitted=0, overflow=0, resync=0, resetDrops=0, taken=0;
    size_t peak=0;
    void Reset(uint64_t value) { resetDrops+=frames.size(); frames.clear(); session=value; recovery=true; }
    bool Push(Frame f) {
        P::Require(f.session==session && session, "queue session mismatch");
        ++received;
        if (recovery && (f.info.flags&7)!=7) { ++resync; return false; }
        if (frames.size()==Bound) {
            ++overflow; resetDrops+=frames.size(); frames.clear(); recovery=true; return false;
        }
        recovery=false; frames.push_back(std::move(f)); ++admitted;
        peak=std::max(peak, frames.size()); return true;
    }
    bool Pop(Frame& f) { if (frames.empty()) return false; f=std::move(frames.front()); frames.pop_front(); ++taken; return true; }
    size_t Size() const { return frames.size(); }
    bool Recovering() const { return recovery; }
};

class GpuWindow {
    static LRESULT CALLBACK WndProc(HWND window, UINT message, WPARAM w, LPARAM l) {
        if (message==WM_CLOSE) { PostQuitMessage(0); return 0; }
        if (message==WM_GETMINMAXINFO) { auto info=reinterpret_cast<MINMAXINFO*>(l);info->ptMinTrackSize={640,320};return 0; }
        if (message==WM_ERASEBKGND) return 1;
        return DefWindowProcW(window,message,w,l);
    }
    ComPtr<IDXGISwapChain1> swap;
    ComPtr<ID3D11VideoProcessorEnumerator> enumerator;
    ComPtr<ID3D11VideoProcessor> processor;
    ComPtr<ID3D11Texture2D> cells;
    T::EvidenceFile* sparse=nullptr;
    bool sparsePending=false,sparseTagged=false;
    uint64_t sparseStart=0;
    struct SparseTag {uint64_t session,sequence,id,source,pts,received,decoded,present,generation;bool shown;} sparseTag{};
    UINT backWidth=0,backHeight=0,codedWidth=0,codedHeight=0;
public:
    HWND window=nullptr;
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    ComPtr<ID3D11VideoDevice> video;
    ComPtr<ID3D11VideoContext> videoContext;
    DXGI_ADAPTER_DESC adapter{};
    uint64_t resizeCount=0, minimized=0, occluded=0, presentBusy=0, staleGeneration=0, surfaceProofs=0, renderSubmissions=0;
    uint64_t sparseSubmitted=0,sparseCompleted=0,sparseBusy=0,sparseSkipped=0;
    explicit GpuWindow(T::EvidenceFile* samples=nullptr):sparse(samples) {
        if(sparse)fprintf(sparse->value,"session,sequence,frame_id,source_ns,pts,receive_ns,decoded_ns,present_ns,presented,generation,nonce,counter,ambiguous,copy_ns,ready_ns\n");
        WNDCLASSW wc{}; wc.lpfnWndProc=WndProc; wc.hInstance=GetModuleHandleW(nullptr); wc.lpszClassName=L"SweetDisplayVisualV1";
        P::Require(RegisterClassW(&wc)!=0, "window class");
        window=CreateWindowExW(0,wc.lpszClassName,L"SweetDisplay — live hardware decode (waiting)",WS_OVERLAPPEDWINDOW,
            60,60,1220,600,nullptr,nullptr,wc.hInstance,nullptr);
        P::Require(window!=nullptr,"visible window creation");
        D3D_FEATURE_LEVEL level{};
        Check(D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_VIDEO_SUPPORT|D3D11_CREATE_DEVICE_BGRA_SUPPORT,
            nullptr,0,D3D11_SDK_VERSION,&device,&level,&context),"hardware D3D11 device");
        ComPtr<ID3D10Multithread> multithread; Check(context.As(&multithread),"D3D multithread interface"); multithread->SetMultithreadProtected(TRUE);
        Check(device.As(&video),"video device"); Check(context.As(&videoContext),"video context");
        ComPtr<IDXGIDevice> dxgi; Check(device.As(&dxgi),"DXGI device"); ComPtr<IDXGIAdapter> a; Check(dxgi->GetAdapter(&a),"adapter");
        Check(a->GetDesc(&adapter),"adapter description"); ComPtr<IDXGIFactory2> factory; Check(a->GetParent(IID_PPV_ARGS(&factory)),"factory");
        DXGI_SWAP_CHAIN_DESC1 desc{}; desc.Format=DXGI_FORMAT_B8G8R8A8_UNORM; desc.SampleDesc.Count=1;
        desc.BufferUsage=DXGI_USAGE_RENDER_TARGET_OUTPUT; desc.BufferCount=2; desc.SwapEffect=DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;
        Check(factory->CreateSwapChainForHwnd(device.Get(),window,&desc,nullptr,nullptr,&swap),"window swapchain");
        Check(factory->MakeWindowAssociation(window,DXGI_MWA_NO_ALT_ENTER),"window association");
        D3D11_TEXTURE2D_DESC cd{}; cd.Width=64; cd.Height=1; cd.MipLevels=1; cd.ArraySize=1; cd.Format=desc.Format;
        cd.SampleDesc.Count=1; cd.Usage=D3D11_USAGE_STAGING; cd.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        Check(device->CreateTexture2D(&cd,nullptr,&cells),"64-pixel diagnostic staging");
        Checkpoint("renderer_initialized");ShowWindow(window,SW_SHOWNOACTIVATE); UpdateWindow(window);
        Clear();
    }
    ~GpuWindow() { if(context)context->ClearState();swap.Reset();if(window) DestroyWindow(window); UnregisterClassW(L"SweetDisplayVisualV1",GetModuleHandleW(nullptr)); }
    bool AsyncSparse() const {return sparse!=nullptr;}
    void TagSparse(const Frame& f,uint64_t decoded,uint64_t present,bool shown){
        P::Require(sparse&&sparsePending&&!sparseTagged,"unique sparse resource ownership");
        sparseTag={f.session,f.sequence,f.info.id,f.info.sourceNs,f.info.pts,f.received,decoded,present,f.generation,shown};sparseTagged=true;
    }
    void PollSparse(){
        if(!sparsePending)return;P::Require(sparseTagged,"sparse copy metadata missing");
        Identity(sparseTag.session,sparseTag.id,sparseTag.generation);Stage("sparse_poll_begin");
        D3D11_MAPPED_SUBRESOURCE map{};HRESULT hr=context->Map(cells.Get(),0,D3D11_MAP_READ,D3D11_MAP_FLAG_DO_NOT_WAIT,&map);
        Stage("sparse_poll_end",0,hr);
        if(hr==DXGI_ERROR_WAS_STILL_DRAWING){++sparseBusy;P::Require(T::Now()-sparseStart<1000000000ULL,"sparse GPU completion deadline");return;}
        Check(hr,"nonblocking sparse diagnostic map");uint32_t nonce=0,counter=0,ambiguous=0;
        const auto bytes=static_cast<const uint8_t*>(map.pData);
        for(UINT i=0;i<64;++i){UINT v=(UINT(bytes[i*4])+bytes[i*4+1]+bytes[i*4+2])/3;
            if(v>=175)(i<32?nonce:counter)|=1u<<(i%32);else if(v>80)++ambiguous;}
        context->Unmap(cells.Get(),0);auto ready=T::Now();const auto& t=sparseTag;
        fprintf(sparse->value,"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%llu,%u,%u,%u,%llu,%llu\n",t.session,t.sequence,t.id,t.source,t.pts,t.received,t.decoded,t.present,unsigned(t.shown),t.generation,nonce,counter,ambiguous,sparseStart,ready);sparse->Flush();
        ++sparseCompleted;sparsePending=sparseTagged=false;Stage("sparse_completed");
    }
    void FinishSparse(){
        if(sparsePending){context->Flush();auto until=T::Now()+500000000ULL;
            while(sparsePending&&T::Now()<until){PollSparse();if(sparsePending)Sleep(1);}}
        P::Require(!sparsePending&&sparseSubmitted==sparseCompleted,"exact sparse completion accounting");
    }
    bool Pump() {
        Stage("pump_begin");MSG msg{};
        for(;;){Stage("peek_begin");BOOL found=PeekMessageW(&msg,nullptr,0,0,PM_REMOVE);Stage("peek_end",found?msg.message:0);
            if(!found)break;if(msg.message==WM_QUIT){Stage("pump_end",1);return false;}
            Stage("dispatch_begin",msg.message);TranslateMessage(&msg);DispatchMessageW(&msg);Stage("dispatch_end",msg.message);
        }Stage("pump_end");return true;
    }
    bool Surface(ComPtr<ID3D11Texture2D>& back) {
        if(IsIconic(window)) return false;
        RECT client{}; P::Require(GetClientRect(window,&client)!=0,"client rectangle");
        UINT w=UINT(client.right),h=UINT(client.bottom); if(!w||!h)return false;
        if(w!=backWidth||h!=backHeight) { Checkpoint("resize_begin");context->ClearState(); Check(swap->ResizeBuffers(2,w,h,DXGI_FORMAT_UNKNOWN,0),"resize buffers"); backWidth=w; backHeight=h; ++resizeCount;Checkpoint("resize_end"); }
        Check(swap->GetBuffer(0,IID_PPV_ARGS(&back)),"back buffer"); return true;
    }
    void Clear() { ComPtr<ID3D11Texture2D> back; if(!Surface(back))return; ComPtr<ID3D11RenderTargetView> rtv;
        Check(device->CreateRenderTargetView(back.Get(),nullptr,&rtv),"clear target"); const float black[4]={0,0,0,1}; context->ClearRenderTargetView(rtv.Get(),black);
        HRESULT hr=DXGI_ERROR_WAS_STILL_DRAWING;for(unsigned i=0;i<50&&hr==DXGI_ERROR_WAS_STILL_DRAWING;++i){hr=swap->Present(0,DXGI_PRESENT_DO_NOT_WAIT);if(hr==DXGI_ERROR_WAS_STILL_DRAWING)Sleep(2);}Check(hr,"clear present"); }
    // Returns actual Present acceptance, not an inference from Decode/ProcessOutput.
    bool Render(IMFSample* sample, UINT cw, UINT ch, const RECT& source, bool& diagnostic,
                uint32_t& nonce, uint32_t& counter, uint32_t& ambiguous,
                std::mutex& presentationGate,const std::atomic<uint64_t>& generation,uint64_t expectedGeneration) {
        ComPtr<IMFMediaBuffer> buffer; Check(sample->GetBufferByIndex(0,&buffer),"decoded buffer");
        ComPtr<IMFDXGIBuffer> dxgi; Check(buffer.As(&dxgi),"require GPU decoded surface (no software fallback)");
        ComPtr<ID3D11Texture2D> input; Check(dxgi->GetResource(IID_PPV_ARGS(&input)),"decoded texture"); UINT subresource=0;
        Check(dxgi->GetSubresourceIndex(&subresource),"decoded array slice"); D3D11_TEXTURE2D_DESC td{}; input->GetDesc(&td);
        P::Require(td.Format==DXGI_FORMAT_NV12 && (td.BindFlags&D3D11_BIND_DECODER) && td.Usage==D3D11_USAGE_DEFAULT && !td.CPUAccessFlags,
            "DXVA NV12 decoder-bound default GPU surface required");
        ComPtr<ID3D11Device> owner; input->GetDevice(&owner); P::Require(owner.Get()==device.Get(),"decoded device identity");
        if(!surfaceProofs)printf("DXVA_SURFACE format=%u bind=%u usage=%u cpu_access=%u array=%u device_identity=1 vendor=%u\n",unsigned(td.Format),td.BindFlags,unsigned(td.Usage),td.CPUAccessFlags,td.ArraySize,adapter.VendorId);
        ++surfaceProofs;
        if(cw!=codedWidth||ch!=codedHeight) {
            D3D11_VIDEO_PROCESSOR_CONTENT_DESC d{}; d.InputFrameFormat=D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;
            d.InputWidth=cw; d.InputHeight=ch; d.OutputWidth=2400; d.OutputHeight=1080; d.Usage=D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;
            enumerator.Reset(); processor.Reset(); Check(video->CreateVideoProcessorEnumerator(&d,&enumerator),"video processor enumerator");
            Check(video->CreateVideoProcessor(enumerator.Get(),0,&processor),"GPU video processor"); codedWidth=cw; codedHeight=ch;
        }
        ComPtr<ID3D11Texture2D> back; if(!Surface(back)){++minimized;diagnostic=false;return false;}
        double scale=std::min(double(backWidth)/2400.0,double(backHeight)/1080.0);
        LONG w=LONG(2400*scale),h=LONG(1080*scale); RECT dest{(LONG(backWidth)-w)/2,(LONG(backHeight)-h)/2,0,0}; dest.right=dest.left+w; dest.bottom=dest.top+h;
        ComPtr<ID3D11RenderTargetView> rtv; Check(device->CreateRenderTargetView(back.Get(),nullptr,&rtv),"render target");
        const float black[4]={0,0,0,1}; context->ClearRenderTargetView(rtv.Get(),black);
        D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC iv{}; iv.ViewDimension=D3D11_VPIV_DIMENSION_TEXTURE2D; iv.Texture2D.ArraySlice=subresource/td.MipLevels;
        ComPtr<ID3D11VideoProcessorInputView> in; Check(video->CreateVideoProcessorInputView(input.Get(),enumerator.Get(),&iv,&in),"NV12 input view");
        D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC ov{}; ov.ViewDimension=D3D11_VPOV_DIMENSION_TEXTURE2D;
        ComPtr<ID3D11VideoProcessorOutputView> out; Check(video->CreateVideoProcessorOutputView(back.Get(),enumerator.Get(),&ov,&out),"swapchain output view");
        videoContext->VideoProcessorSetStreamFrameFormat(processor.Get(),0,D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);
        videoContext->VideoProcessorSetStreamSourceRect(processor.Get(),0,TRUE,&source);
        videoContext->VideoProcessorSetStreamDestRect(processor.Get(),0,TRUE,&dest);
        D3D11_VIDEO_PROCESSOR_COLOR_SPACE cs{}; cs.YCbCr_Matrix=1; videoContext->VideoProcessorSetStreamColorSpace(processor.Get(),0,&cs);
        cs={}; cs.RGB_Range=0; videoContext->VideoProcessorSetOutputColorSpace(processor.Get(),&cs);
        D3D11_VIDEO_PROCESSOR_STREAM stream{}; stream.Enable=TRUE; stream.pInputSurface=in.Get();
        Checkpoint("blit_begin");Check(videoContext->VideoProcessorBlt(processor.Get(),out.Get(),0,1,&stream),"NV12 to letterboxed BGRA GPU blit");Checkpoint("blit_end");
        ++renderSubmissions;
        if(diagnostic&&sparsePending){++sparseSkipped;diagnostic=false;}
        if(diagnostic) {
            for(UINT row=0;row<2;++row) for(UINT bit=0;bit<32;++bit) {
                UINT x=UINT(dest.left)+(32+bit*20+8)*UINT(w)/2400, y=UINT(dest.top)+(row?96:48)*UINT(h)/1080;
                D3D11_BOX box{x,y,0,x+1,y+1,1}; context->CopySubresourceRegion(cells.Get(),0,row*32+bit,0,0,back.Get(),0,&box);
            }
            if(sparse){sparsePending=true;sparseTagged=false;sparseStart=T::Now();++sparseSubmitted;Stage("sparse_copy_queued");}
            else {
            // Legacy standalone ResourceProbe only. The simulator always supplies
            // the asynchronous journal and never takes this blocking branch.
            Checkpoint("diagnostic_map_begin");D3D11_MAPPED_SUBRESOURCE map{}; Check(context->Map(cells.Get(),0,D3D11_MAP_READ,0,&map),"sparse rendered diagnostic");Checkpoint("diagnostic_map_end");
            nonce=counter=ambiguous=0; const auto bytes=static_cast<const uint8_t*>(map.pData);
            for(UINT i=0;i<64;++i) { UINT v=(UINT(bytes[i*4])+bytes[i*4+1]+bytes[i*4+2])/3;
                if(v>=175) (i<32?nonce:counter)|=1u<<(i%32); else if(v>80)++ambiguous; }
            context->Unmap(cells.Get(),0);
            }
        }
        // Only the nonblocking Present is serialized with session invalidation.
        // Network receipt never holds this lock while waiting on decoder/GPU work.
        std::lock_guard<std::mutex> guard(presentationGate);
        if(generation.load()!=expectedGeneration){++staleGeneration;if(!sparse)diagnostic=false;return false;}
        Checkpoint("present_begin");HRESULT hr=swap->Present(0,DXGI_PRESENT_DO_NOT_WAIT);Stage("present_result",0,hr);Checkpoint("present_end");
        if(hr==DXGI_ERROR_WAS_STILL_DRAWING) { ++presentBusy; return false; }
        if(hr==DXGI_STATUS_OCCLUDED) { ++occluded; return false; }
        Check(hr,"visible present");Checkpoint("present_success"); return true;
    }
};

class Decoder {
    ComPtr<IMFTransform> mft;
    ComPtr<IMFDXGIDeviceManager> manager;
    std::map<LONGLONG,Frame> pending;
    uint64_t session=0;
    UINT width=0,height=0;
    RECT aperture{};
    T::EvidenceFile& ledger;
    GpuWindow& gpu;
    uint64_t lastDiagnostic=0;
    bool needsIdr=true;
    std::mutex& presentationGate;
    const std::atomic<uint64_t>& generation;
    void SelectOutput() {
        for(DWORD i=0;;++i) { ComPtr<IMFMediaType> t; Check(mft->GetOutputAvailableType(0,i,&t),"decoder output type");
            GUID sub{}; Check(t->GetGUID(MF_MT_SUBTYPE,&sub),"decoder subtype"); if(sub!=MFVideoFormat_NV12)continue;
            Check(mft->SetOutputType(0,t.Get(),0),"D3D NV12 output negotiation"); return; }
    }
    void Geometry() {
        ComPtr<IMFMediaType> t; Check(mft->GetOutputCurrentType(0,&t),"output type");
        Check(MFGetAttributeSize(t.Get(),MF_MT_FRAME_SIZE,&width,&height),"coded size");
        aperture={0,0,LONG(width),LONG(height)}; MFVideoArea area{}; UINT n=0;
        HRESULT hr=t->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,reinterpret_cast<BYTE*>(&area),sizeof(area),&n);
        if(hr==MF_E_ATTRIBUTENOTFOUND)hr=t->GetBlob(MF_MT_GEOMETRIC_APERTURE,reinterpret_cast<BYTE*>(&area),sizeof(area),&n);
        if(hr!=MF_E_ATTRIBUTENOTFOUND) { Check(hr,"visible aperture"); P::Require(n==sizeof(area)&&!area.OffsetX.fract&&!area.OffsetY.fract,"integer aperture");
            aperture={area.OffsetX.value,area.OffsetY.value,area.OffsetX.value+area.Area.cx,area.OffsetY.value+area.Area.cy}; }
        P::Require(aperture.left>=0&&aperture.top>=0&&aperture.right<=LONG(width)&&aperture.bottom<=LONG(height)&&
            aperture.right-aperture.left==2400&&aperture.bottom-aperture.top==1080,"decoded real visible geometry");
    }
public:
    uint64_t submitted=0,decoded=0,presented=0,renderDrops=0,resetDrops=0,resets=0,diagnostics=0,lastPresentedSession=0;
    size_t pendingPeak=0;
    explicit Decoder(GpuWindow& g,T::EvidenceFile& log,std::mutex& gate,const std::atomic<uint64_t>& epoch):ledger(log),gpu(g),presentationGate(gate),generation(epoch) {
        fprintf(ledger.value,"session,sequence,frame_id,source_ns,pts,receive_ns,decoded_ns,present_ns,presented,diagnostic,nonce,counter,ambiguous,pending,generation\n"); ledger.Flush();
        UINT token=0; Check(MFCreateDXGIDeviceManager(&token,&manager),"DXGI device manager"); Check(manager->ResetDevice(gpu.device.Get(),token),"manager hardware device");
    }
    void Reset(uint64_t value) {
        Checkpoint("decoder_reset_begin");
        if(mft) { Check(mft->ProcessMessage(MFT_MESSAGE_COMMAND_FLUSH,0),"decoder flush"); Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING,0),"decoder end"); }
        resetDrops+=pending.size(); pending.clear(); mft.Reset(); session=value; ++resets; needsIdr=true; lastDiagnostic=0; lastPresentedSession=0;
        SetWindowTextW(gpu.window,L"SweetDisplay | session reset — waiting for new IDR; previous content invalid");gpu.Clear();
        Checkpoint("decoder_released");if(!value){Stage("decoder_reset_end");return;}
        Check(CoCreateInstance(CLSID_CMSH264DecoderMFT,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&mft)),"Microsoft H264 decoder MFT");
        ComPtr<IMFAttributes> a; Check(mft->GetAttributes(&a),"decoder attributes"); UINT aware=0; Check(a->GetUINT32(MF_SA_D3D11_AWARE,&aware),"D3D11 aware"); P::Require(aware==1,"D3D11 awareness required");
        Check(a->SetUINT32(MF_LOW_LATENCY,TRUE),"decoder low latency");
        // AVDecVideoAcceleration_H264 is DirectShow-only. It is deliberately NOT
        // used as fake MF hardware proof. Never detach this non-null manager and
        // renegotiate a CPU type after MF_E_UNSUPPORTED_D3D_TYPE.
        Check(mft->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER,reinterpret_cast<ULONG_PTR>(manager.Get())),"decoder D3D manager");
        ComPtr<IMFMediaType> input; Check(MFCreateMediaType(&input),"H264 type"); Check(input->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video),"video major");
        Check(input->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H264),"H264 subtype"); Check(mft->SetInputType(0,input.Get(),0),"hardware H264 input");
        SelectOutput(); Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING,0),"begin decode"); Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM,0),"start decode");Checkpoint("decoder_initialized");Stage("decoder_reset_end");
    }
    void Pull() {
        for(unsigned loop=0;loop<32;++loop) {
            MFT_OUTPUT_STREAM_INFO info{}; Check(mft->GetOutputStreamInfo(0,&info),"output allocation");
            P::Require((info.dwFlags&MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)!=0,"decoder must allocate GPU output; no CPU fallback");
            Checkpoint("decode_output_begin");MFT_OUTPUT_DATA_BUFFER out{}; DWORD status=0; HRESULT hr=mft->ProcessOutput(0,1,&out,&status);Stage("decode_output_result",0,hr);Checkpoint("decode_output_end");
            if(out.pEvents)out.pEvents->Release(); ComPtr<IMFSample> sample; sample.Attach(out.pSample);
            if(hr==MF_E_TRANSFORM_STREAM_CHANGE) { SelectOutput(); continue; }
            if(hr==MF_E_TRANSFORM_NEED_MORE_INPUT)return;
            Check(hr,"hardware decoder output"); P::Require(sample!=nullptr,"nonempty decode"); Geometry();
            LONGLONG pts=0; Check(sample->GetSampleTime(&pts),"decoded PTS"); auto it=pending.find(pts);
            P::Require(it!=pending.end()&&it->second.session==session,"decoded frame/session identity");
            auto f=std::move(it->second); pending.erase(it); ++decoded;Identity(f.session,f.info.id,f.generation);Checkpoint("decoded_surface");
            auto now=T::Now(); bool diagnostic=!lastDiagnostic||now-lastDiagnostic>=500000000ULL; uint32_t nonce=0,counter=0,ambiguous=0;
            Stage("render_begin");bool shown=gpu.Render(sample.Get(),width,height,aperture,diagnostic,nonce,counter,ambiguous,presentationGate,generation,f.generation);Stage("render_end",unsigned(shown));
            auto present=T::Now();if(diagnostic&&gpu.AsyncSparse())gpu.TagSparse(f,now,present,shown);
            if(shown){++presented;lastPresentedSession=session;}else ++renderDrops; if(diagnostic){lastDiagnostic=now;++diagnostics;}
            fprintf(ledger.value,"%llu,%llu,%llu,%llu,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%zu,%llu\n",f.session,f.sequence,f.info.id,f.info.sourceNs,f.info.pts,
                f.received,now,present,unsigned(shown),unsigned(diagnostic&&!gpu.AsyncSparse()),nonce,counter,ambiguous,pending.size(),f.generation); ledger.Flush();
        }
        throw Failure("bounded output loop",E_FAIL);
    }
    void Push(Frame f) {
        Identity(f.session,f.info.id,f.generation);Checkpoint("submission_begin");
        P::Require(f.session==session&&f.info.width==2400&&f.info.height==1080,"decoder session/format");
        P::Require(pending.size()<16,"bounded decoder pending exhausted");
        if(needsIdr)P::Require((f.info.flags&7)==7,"first decode IDR/SPS/PPS");
        ComPtr<IMFSample> sample; ComPtr<IMFMediaBuffer> b; Check(MFCreateSample(&sample),"compressed sample"); Check(MFCreateMemoryBuffer(DWORD(f.au.size()),&b),"compressed bytes");
        BYTE* bytes=nullptr; Check(b->Lock(&bytes,nullptr,nullptr),"compressed buffer lock"); memcpy(bytes,f.au.data(),f.au.size()); Check(b->Unlock(),"compressed buffer unlock");
        Check(b->SetCurrentLength(DWORD(f.au.size())),"compressed length"); Check(sample->AddBuffer(b.Get()),"compressed attach"); Check(sample->SetSampleTime(LONGLONG(f.info.pts)),"source PTS");
        if(f.info.flags&1)Check(sample->SetUINT32(MFSampleExtension_CleanPoint,TRUE),"IDR sample");
        Stage("decode_input_begin");HRESULT hr=mft->ProcessInput(0,sample.Get(),0);Stage("decode_input_end",0,hr); if(hr==MF_E_NOTACCEPTING){Pull();Identity(f.session,f.info.id,f.generation);Stage("decode_input_begin");hr=mft->ProcessInput(0,sample.Get(),0);Stage("decode_input_end",0,hr);} Check(hr,"hardware decoder input");Stage("submission_accepted");
        needsIdr=false; f.au.clear(); P::Require(pending.emplace(LONGLONG(f.info.pts),std::move(f)).second,"unique pending PTS"); ++submitted; pendingPeak=std::max(pendingPeak,pending.size()); Pull();
    }
    void Drain() { if(!mft)return; Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM,0),"decode EOS"); Check(mft->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN,0),"decode drain"); Pull(); P::Require(pending.empty(),"decode exact drain count"); }
};
}
