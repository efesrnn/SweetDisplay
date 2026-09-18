// Diagnostic-only frame observation. Disabled unless an ETW controller enables it.
#pragma once
#include "../diagnostics/FrameEvents.h"
#include <evntprov.h>
#include <atomic>
namespace SweetDisplay {
class FrameDiagnostics {
    REGHANDLE registration_=0;
    uint64_t previousQpc_=0;
    inline static std::atomic<uint64_t> nextId_{0};
    inline static std::atomic<bool> captureClaimed_{false};
    ULONG Write(USHORT id, uint64_t keyword, const void* data, ULONG bytes,
                const void* extra=nullptr, ULONG extraBytes=0) noexcept {
        EVENT_DESCRIPTOR event{};
        event.Id=id; event.Version=Diagnostic::Version; event.Level=4; event.Keyword=keyword;
        EVENT_DATA_DESCRIPTOR descriptors[2]{};
        EventDataDescCreate(&descriptors[0],data,bytes);
        if (extraBytes) EventDataDescCreate(&descriptors[1],extra,extraBytes);
        return EventWrite(registration_,&event,extraBytes ? 2 : 1,descriptors);
    }
    void Fail(uint64_t id, HRESULT hr, uint32_t stage) noexcept {
        Diagnostic::Error error{id,hr,stage};
        Write(Diagnostic::ErrorEvent,Diagnostic::MetadataKeyword,&error,sizeof(error));
    }
public:
    FrameDiagnostics() noexcept {
        EventRegister(&Diagnostic::Provider,nullptr,nullptr,&registration_);
    }
    ~FrameDiagnostics() {
        if (registration_) EventUnregister(registration_);
    }
    void Observe(IDXGIResource* surface, const IDDCX_METADATA& meta,
                 const Microsoft::IndirectDisp::Direct3DDevice& device) noexcept {
        if (!registration_ || !EventProviderEnabled(registration_,4,Diagnostic::MetadataKeyword)) return;
        const uint64_t id=++nextId_;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
        HRESULT hr=surface->QueryInterface(IID_PPV_ARGS(&texture));
        if (FAILED(hr)) { Fail(id,hr,1); return; }
        D3D11_TEXTURE2D_DESC desc{};
        texture->GetDesc(&desc);
        LARGE_INTEGER now{},frequency{};
        QueryPerformanceCounter(&now); QueryPerformanceFrequency(&frequency);
        Diagnostic::Frame frame{};
        frame.frameId=id; frame.acquiredQpc=now.QuadPart; frame.qpcFrequency=frequency.QuadPart;
        frame.presentQpc=meta.PresentDisplayQPCTime;
        frame.intervalQpc=previousQpc_ ? frame.acquiredQpc-previousQpc_ : 0;
        previousQpc_=frame.acquiredQpc;
        frame.width=desc.Width; frame.height=desc.Height; frame.format=desc.Format;
        frame.presentationFrameNumber=meta.PresentationFrameNumber;
        frame.adapterLow=device.AdapterLuid.LowPart; frame.adapterHigh=device.AdapterLuid.HighPart;
        Write(Diagnostic::FrameEvent,Diagnostic::MetadataKeyword,&frame,sizeof(frame));
        if (!EventProviderEnabled(registration_,4,Diagnostic::CaptureKeyword)) return;
        bool expected=false;
        if (!captureClaimed_.compare_exchange_strong(expected,true)) return;
        // Never read protected content; exactly one capture attempt per driver process.
        if (meta.HwProtectedSurface) { Fail(id,E_ACCESSDENIED,2); return; }
        if (desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM && desc.Format!=DXGI_FORMAT_B8G8R8X8_UNORM &&
            desc.Format!=DXGI_FORMAT_R8G8B8A8_UNORM) { Fail(id,E_NOTIMPL,3); return; }
        const uint64_t bytes=uint64_t(desc.Width)*desc.Height*4;
        if (!bytes || bytes>Diagnostic::MaxBytes || desc.SampleDesc.Count!=1 || desc.ArraySize!=1) {
            Fail(id,E_INVALIDARG,4); return;
        }
        D3D11_TEXTURE2D_DESC stagingDesc=desc;
        stagingDesc.Usage=D3D11_USAGE_STAGING;
        stagingDesc.BindFlags=0; stagingDesc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
        stagingDesc.MiscFlags=0;
        Microsoft::WRL::ComPtr<ID3D11Texture2D> staging;
        hr=device.Device->CreateTexture2D(&stagingDesc,nullptr,&staging);
        if (FAILED(hr)) { Fail(id,hr,5); return; }
        // Both operations occur before the next ReleaseAndAcquire; Map waits for this copy.
        device.DeviceContext->CopyResource(staging.Get(),texture.Get());
        D3D11_MAPPED_SUBRESOURCE mapped{};
        hr=device.DeviceContext->Map(staging.Get(),0,D3D11_MAP_READ,0,&mapped);
        if (FAILED(hr)) { Fail(id,hr,6); return; }
        try {
            std::vector<unsigned char> pixels(static_cast<size_t>(bytes));
            const uint32_t rowBytes=desc.Width*4;
            for (UINT row=0;row<desc.Height;++row)
                memcpy(pixels.data()+size_t(row)*rowBytes,
                       static_cast<unsigned char*>(mapped.pData)+size_t(row)*mapped.RowPitch,rowBytes);
            device.DeviceContext->Unmap(staging.Get(),0);
            mapped.pData=nullptr;
            ULONG status=Write(Diagnostic::BeginEvent,Diagnostic::CaptureKeyword,&frame,sizeof(frame));
            for (uint32_t offset=0;status==ERROR_SUCCESS && offset<pixels.size();) {
                const uint32_t length=(std::min)(Diagnostic::ChunkBytes,static_cast<uint32_t>(pixels.size()-offset));
                Diagnostic::Chunk chunk{id,offset,length};
                status=Write(Diagnostic::ChunkEvent,Diagnostic::CaptureKeyword,&chunk,sizeof(chunk),pixels.data()+offset,length);
                offset+=length;
            }
            if (status!=ERROR_SUCCESS) { Fail(id,HRESULT_FROM_WIN32(status),7); return; }
            Diagnostic::End end{id,Diagnostic::Hash(pixels.data(),pixels.size()),static_cast<uint32_t>(bytes),rowBytes};
            status=Write(Diagnostic::EndEvent,Diagnostic::CaptureKeyword,&end,sizeof(end));
            if (status!=ERROR_SUCCESS) Fail(id,HRESULT_FROM_WIN32(status),8);
        } catch (...) {
            if (mapped.pData) device.DeviceContext->Unmap(staging.Get(),0);
            Fail(id,E_OUTOFMEMORY,9);
        }
    }
};
}

