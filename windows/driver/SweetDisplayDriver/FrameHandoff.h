// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include "Driver.h"
#include "../../shared/FrameHandoffProtocol.h"
#include <mutex>
#include <atomic>

namespace SweetDisplay {
class FrameHandoff {
    using D3D=Microsoft::IndirectDisp::Direct3DDevice;
    template<class T> using Ptr=Microsoft::WRL::ComPtr<T>;
    struct Connection {
        WDFFILEOBJECT owner=nullptr;
        Handoff::Connect config{};
        Ptr<ID3D11Texture2D> textures[Handoff::Capacity];
        Ptr<IDXGIKeyedMutex> mutexes[Handoff::Capacity];
        Handoff::Queue queue;
        uint64_t sourceBase=0;
    };
    std::mutex guard;
    std::shared_ptr<D3D> device;
    std::shared_ptr<Connection> connection;
    Handoff::State state=Handoff::Packet<Handoff::State>();
    std::atomic<uint64_t> source{0},contention{0};
public:
    FrameHandoff() { LARGE_INTEGER q{};QueryPerformanceFrequency(&q);state.frequency=q.QuadPart; }
    void Start(std::shared_ptr<D3D> d) {
        std::lock_guard<std::mutex> lock(guard);
        connection.reset();device=std::move(d);state.active=0;state.width=state.height=state.format=0;
        LARGE_INTEGER q{};QueryPerformanceCounter(&q);state.epoch=q.QuadPart;state.adapter=device->AdapterLuid;state.error=0;
    }
    void Stop() {
        std::lock_guard<std::mutex> lock(guard); connection.reset();device.reset();state.active=0;
    }
    void Disconnect(WDFFILEOBJECT owner) {
        std::lock_guard<std::mutex> lock(guard);if(connection&&connection->owner==owner) connection.reset();
    }
    void Observe(IDXGIResource* resource,const IDDCX_METADATA& meta) {
        uint64_t id=++source;LARGE_INTEGER now{};QueryPerformanceCounter(&now);
        std::unique_lock<std::mutex> lock(guard,std::try_to_lock);
        if(!lock.owns_lock()){++contention;return;}
        Ptr<ID3D11Texture2D> texture;
        if(FAILED(resource->QueryInterface(IID_PPV_ARGS(&texture)))) {if(connection)++connection->queue.stats.invalid;return;}
        D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
        state.width=desc.Width;state.height=desc.Height;state.format=desc.Format;state.active=1;
        if(!connection){++state.noHost;return;}
        auto& c=*connection; auto& queue=c.queue;
        if(meta.HwProtectedSurface||desc.Width!=c.config.width||desc.Height!=c.config.height||desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM||desc.SampleDesc.Count!=1) {
            ++queue.stats.invalid;state.error=E_INVALIDARG;return;
        }
        int slot=queue.ProducerSlot();
        if(slot<0){++queue.stats.busy;return;}
        HRESULT hr=c.mutexes[slot]->AcquireSync(queue.slots[slot].key,0);
        if(hr==WAIT_TIMEOUT){++queue.stats.busy;return;}
        if(hr!=S_OK){state.error=hr;connection.reset();return;}
        device->DeviceContext->CopyResource(c.textures[slot].Get(),texture.Get());
        device->DeviceContext->Flush();
        hr=c.mutexes[slot]->ReleaseSync(1);
        if(FAILED(hr)){state.error=hr;connection.reset();return;}
        Handoff::Frame frame{};frame.id=id;frame.qpc=now.QuadPart;frame.epoch=state.epoch;
        frame.presentation=meta.PresentationFrameNumber;frame.width=desc.Width;frame.height=desc.Height;frame.format=desc.Format;
        queue.Publish(slot,frame);
    }
    NTSTATUS Control(WDFFILEOBJECT owner,ULONG code,const void* input,size_t inSize,void* output,size_t outSize,size_t& written) {
        using namespace Handoff;
        if(!owner) return STATUS_ACCESS_DENIED;
        if(code==ConnectIo) {
            if(!input||inSize!=sizeof(Connect))return STATUS_INVALID_PARAMETER;
            Connect config{};memcpy(&config,input,sizeof(config));
            if(!ValidConnect(config,inSize))return STATUS_INVALID_PARAMETER;
            std::shared_ptr<D3D> d;
            {
                std::lock_guard<std::mutex> lock(guard);
                if(connection)return STATUS_DEVICE_BUSY;
                if(!device||!state.active||config.epoch!=state.epoch||memcmp(&config.adapter,&state.adapter,sizeof(LUID))||
                    config.width!=state.width||config.height!=state.height||config.format!=state.format) return STATUS_DEVICE_NOT_READY;
                d=device;
            }
            // D3D device methods are thread safe; no immediate-context work here.
            // Opening resources can be expensive, so keep it off the swapchain thread/lock.
            auto c=std::make_shared<Connection>(); c->config=config;c->owner=owner;
            Ptr<ID3D11Device1> d1;HRESULT hr=d->Device.As(&d1);
            for(uint32_t i=0;SUCCEEDED(hr)&&i<Capacity;++i) {
                hr=d1->OpenSharedResourceByName(config.names[i],DXGI_SHARED_RESOURCE_READ|DXGI_SHARED_RESOURCE_WRITE,IID_PPV_ARGS(&c->textures[i]));
                if(FAILED(hr))break;
                D3D11_TEXTURE2D_DESC desc{};c->textures[i]->GetDesc(&desc);
                if(desc.Width!=config.width||desc.Height!=config.height||desc.Format!=DXGI_FORMAT(config.format)||desc.MipLevels!=1||desc.ArraySize!=1||
                    desc.SampleDesc.Count!=1||desc.Usage!=D3D11_USAGE_DEFAULT||desc.CPUAccessFlags!=0||
                    !(desc.MiscFlags&D3D11_RESOURCE_MISC_SHARED_NTHANDLE)||!(desc.MiscFlags&D3D11_RESOURCE_MISC_SHARED_KEYEDMUTEX)) return STATUS_INVALID_PARAMETER;
                hr=c->textures[i].As(&c->mutexes[i]);
            }
            std::lock_guard<std::mutex> lock(guard);
            if(FAILED(hr)){state.error=hr;return STATUS_UNSUCCESSFUL;}
            if(connection||device!=d||config.epoch!=state.epoch)return STATUS_DEVICE_BUSY;
            c->sourceBase=source.load();connection=std::move(c);state.error=0;return STATUS_SUCCESS;
        }
        std::lock_guard<std::mutex> lock(guard);
        if(code==StateIo||code==FetchIo) {
            if(inSize!=0||!output||outSize!=sizeof(State))return STATUS_INVALID_PARAMETER;
            State reply=state;reply.totalSource=source.load();reply.contention=contention.load();reply.connected=connection?1:0;reply.frame={};reply.stats={};
            if(code==FetchIo) {
                if(!connection||connection->owner!=owner)return STATUS_DEVICE_NOT_CONNECTED;
                int slot=connection->queue.Fetch();if(slot>=0)reply.frame=connection->queue.slots[slot].frame;
            }
            if(connection){reply.stats=connection->queue.stats;reply.stats.source=reply.totalSource-connection->sourceBase;}
            memcpy(output,&reply,sizeof(reply));written=sizeof(reply);return STATUS_SUCCESS;
        }
        if(!connection||connection->owner!=owner)return STATUS_DEVICE_NOT_CONNECTED;
        if(code==DisconnectIo) {if(inSize||outSize)return STATUS_INVALID_PARAMETER;connection.reset();return STATUS_SUCCESS;}
        if(code==AckIo) {
            if(!input||inSize!=sizeof(Ack)||outSize)return STATUS_INVALID_PARAMETER;
            Ack ack{};memcpy(&ack,input,sizeof(ack));
            if(!ValidPacket(ack,inSize)||ack.reserved||!connection->queue.Acknowledge(ack))return STATUS_INVALID_PARAMETER;
            return STATUS_SUCCESS;
        }
        return STATUS_INVALID_DEVICE_REQUEST;
    }
};
}

