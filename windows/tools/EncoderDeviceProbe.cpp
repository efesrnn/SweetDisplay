// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
// Read-only adapter-specific hardware MFT activation/negotiation, not encode proof.
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_4.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <codecapi.h>
#include <wrl.h>
#include <cstdio>
#include <stdexcept>
using Microsoft::WRL::ComPtr;
static void Check(HRESULT hr,const char* label){printf("%s=0x%08lX\n",label,ULONG(hr));if(FAILED(hr))throw std::runtime_error(label);}
static void Attr(IMFAttributes* a,const GUID& key,const char* label){UINT32 v=0;HRESULT hr=a->GetUINT32(key,&v);printf("%s hr=0x%08lX value=%u\n",label,ULONG(hr),v);}
static void Text(IMFAttributes* a,const GUID& key,const char* label){wchar_t* v=nullptr;UINT32 n=0;HRESULT hr=a->GetAllocatedString(key,&v,&n);printf("%s hr=0x%08lX value=%ls\n",label,ULONG(hr),v?v:L"");CoTaskMemFree(v);}
static ComPtr<IMFMediaType> Type(GUID subtype,bool output){ComPtr<IMFMediaType> t;Check(MFCreateMediaType(&t),"MFCreateMediaType");Check(t->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video),"major");Check(t->SetGUID(MF_MT_SUBTYPE,subtype),"subtype");Check(MFSetAttributeSize(t.Get(),MF_MT_FRAME_SIZE,800,360),"size");Check(MFSetAttributeRatio(t.Get(),MF_MT_FRAME_RATE,10,1),"rate");Check(MFSetAttributeRatio(t.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1),"aspect");Check(t->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive),"progressive");if(output){Check(t->SetUINT32(MF_MT_AVG_BITRATE,2000000),"bitrate");Check(t->SetUINT32(MF_MT_MPEG2_PROFILE,eAVEncH264VProfile_Main),"profile");}return t;}
int main(){
 HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(co))return 1;
 HRESULT mf=MFStartup(MF_VERSION);if(FAILED(mf)){CoUninitialize();return 1;}
 int result=0;
 try{
  ComPtr<IDXGIFactory4> factory;Check(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"factory");
  for(UINT i=0;;++i){
   ComPtr<IDXGIAdapter1> adapter;HRESULT hr=factory->EnumAdapters1(i,&adapter);if(hr==DXGI_ERROR_NOT_FOUND)break;Check(hr,"adapter");
   DXGI_ADAPTER_DESC1 desc{};Check(adapter->GetDesc1(&desc),"description");
   printf("ADAPTER index=%u name=%ls luid=%08lx:%08lx vendor=%04x device=%04x flags=%u\n",i,desc.Description,ULONG(desc.AdapterLuid.HighPart),desc.AdapterLuid.LowPart,desc.VendorId,desc.DeviceId,desc.Flags);
   if(desc.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)continue;
   ComPtr<IMFAttributes> filter;Check(MFCreateAttributes(&filter,1),"filter");Check(filter->SetBlob(MFT_ENUM_ADAPTER_LUID,reinterpret_cast<BYTE*>(&desc.AdapterLuid),sizeof(LUID)),"LUID filter");
   MFT_REGISTER_TYPE_INFO out{MFMediaType_Video,MFVideoFormat_H264};IMFActivate** list=nullptr;UINT32 count=0;
   Check(MFTEnum2(MFT_CATEGORY_VIDEO_ENCODER,MFT_ENUM_FLAG_HARDWARE|MFT_ENUM_FLAG_SORTANDFILTER,nullptr,&out,filter.Get(),&list,&count),"MFTEnum2 hardware-only");
   printf("adapter_hardware_h264_count=%u\n",count);
   for(UINT32 j=0;j<count;++j){
    printf("MFT adapter=%u index=%u\n",i,j);Text(list[j],MFT_FRIENDLY_NAME_Attribute,"name");Text(list[j],MFT_ENUM_HARDWARE_URL_Attribute,"hardware_url");
    GUID id{};hr=list[j]->GetGUID(MFT_TRANSFORM_CLSID_Attribute,&id);wchar_t guid[40]{};StringFromGUID2(id,guid,40);printf("clsid=%ls hr=0x%08lX\n",guid,ULONG(hr));
    ComPtr<IMFTransform> mft;
    try{
     Check(list[j]->ActivateObject(IID_PPV_ARGS(&mft)),"ActivateObject");ComPtr<IMFAttributes> attributes;Check(mft->GetAttributes(&attributes),"attributes");
     Attr(attributes.Get(),MF_TRANSFORM_ASYNC,"async");Attr(attributes.Get(),MF_SA_D3D11_AWARE,"d3d11_aware");Check(attributes->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK,TRUE),"unlock_async");
     ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
     Check(D3D11CreateDevice(adapter.Get(),D3D_DRIVER_TYPE_UNKNOWN,nullptr,D3D11_CREATE_DEVICE_BGRA_SUPPORT|D3D11_CREATE_DEVICE_VIDEO_SUPPORT,nullptr,0,D3D11_SDK_VERSION,&device,nullptr,&context),"D3D11 video device");
     ComPtr<IMFDXGIDeviceManager> manager;UINT token=0;Check(MFCreateDXGIDeviceManager(&token,&manager),"DXGI manager");Check(manager->ResetDevice(device.Get(),token),"manager ResetDevice");Check(mft->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER,reinterpret_cast<ULONG_PTR>(manager.Get())),"SET_D3D_MANAGER");
     auto output=Type(MFVideoFormat_H264,true);auto input=Type(MFVideoFormat_NV12,false);
     Check(mft->SetOutputType(0,output.Get(),0),"SetOutput H264 800x360@10");Check(mft->SetInputType(0,input.Get(),0),"SetInput NV12 800x360@10");
     printf("NEGOTIATION_PASS adapter=%u index=%u; actual encoding NOT YET TESTED; software fallback disabled\n",i,j);
    }catch(const std::exception& e){printf("CANDIDATE_FAILED stage=%s\n",e.what());}
    if(mft){ComPtr<IMFShutdown> shutdown;if(SUCCEEDED(mft.As(&shutdown)))printf("Shutdown=0x%08lX\n",ULONG(shutdown->Shutdown()));}
    printf("activation_shutdown=0x%08lX\n",ULONG(list[j]->ShutdownObject()));list[j]->Release();
   }
   CoTaskMemFree(list);
  }
 }catch(const std::exception& e){fprintf(stderr,"PROBE_FAILED %s\n",e.what());result=1;}
 MFShutdown();CoUninitialize();return result;
}
