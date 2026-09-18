// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
// Offline independent Microsoft H.264 decoder. No desktop capture or transport.
#define NOMINMAX
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mftransform.h>
#include <mferror.h>
#include <wmcodecdsp.h>
#include <wrl.h>
#include <cstdio>
#include <memory>
#include <string>
#include <vector>
#include <stdexcept>
#include <algorithm>
using Microsoft::WRL::ComPtr;
struct Error:std::runtime_error{HRESULT hr;Error(const char* s,HRESULT h):std::runtime_error(s),hr(h){}};
static void Check(HRESULT hr,const char* what){if(FAILED(hr))throw Error(what,hr);}
struct Close {void operator()(FILE* f)const{if(f)fclose(f);}};using File=std::unique_ptr<FILE,Close>;
static File Open(const std::wstring& path,const wchar_t* mode){FILE* f=nullptr;if(_wfopen_s(&f,path.c_str(),mode))throw Error("evidence open",E_FAIL);return File(f);}
class Decoder {
 ComPtr<IMFTransform> mft;File csv;UINT width,height;uint64_t count=0;LONGLONG previous=-1;DWORD stride=0;bool typed=false;
 void SelectOutput(){
  for(DWORD i=0;;++i){ComPtr<IMFMediaType> t;Check(mft->GetOutputAvailableType(0,i,&t),"decoder available output");GUID subtype{};Check(t->GetGUID(MF_MT_SUBTYPE,&subtype),"decoder subtype");if(subtype!=MFVideoFormat_NV12)continue;Check(mft->SetOutputType(0,t.Get(),0),"decoder NV12 output");typed=true;return;}
 }
 void Pixels(IMFSample* sample){
  ComPtr<IMFMediaType> type;Check(mft->GetOutputCurrentType(0,&type),"decoded current type");UINT w=0,h=0;Check(MFGetAttributeSize(type.Get(),MF_MT_FRAME_SIZE,&w,&h),"decoded geometry");
  if(!count){printf("decoded_coded_size=%ux%u expected_visible=%ux%u\n",w,h,width,height);for(auto key:{MF_MT_MINIMUM_DISPLAY_APERTURE,MF_MT_GEOMETRIC_APERTURE}){MFVideoArea area{};UINT size=0;HRESULT hr=type->GetBlob(key,reinterpret_cast<BYTE*>(&area),sizeof(area),&size);printf("aperture=%s hr=0x%08lX bytes=%u offset=%d+%u/65536,%d+%u/65536 size=%ldx%ld\n",key==MF_MT_MINIMUM_DISPLAY_APERTURE?"minimum":"geometric",ULONG(hr),size,area.OffsetX.value,area.OffsetX.fract,area.OffsetY.value,area.OffsetY.fract,area.Area.cx,area.Area.cy);}}
  // Decoder allocation may be macroblock padded. Validate the documented visible
  // aperture, never substitute the caller's expected dimensions for bitstream facts.
  UINT xOffset=0,yOffset=0,visibleWidth=w,visibleHeight=h;MFVideoArea area{};UINT areaSize=0;
  HRESULT aperture=type->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE,reinterpret_cast<BYTE*>(&area),sizeof(area),&areaSize);
  if(aperture==MF_E_ATTRIBUTENOTFOUND)aperture=type->GetBlob(MF_MT_GEOMETRIC_APERTURE,reinterpret_cast<BYTE*>(&area),sizeof(area),&areaSize);
  if(aperture!=MF_E_ATTRIBUTENOTFOUND){Check(aperture,"decoded aperture");if(areaSize!=sizeof(area)||area.OffsetX.fract||area.OffsetY.fract||area.OffsetX.value<0||area.OffsetY.value<0||area.Area.cx<=0||area.Area.cy<=0)throw Error("invalid decoded aperture",E_FAIL);xOffset=area.OffsetX.value;yOffset=area.OffsetY.value;visibleWidth=area.Area.cx;visibleHeight=area.Area.cy;}
  if(visibleWidth!=width||visibleHeight!=height||uint64_t(xOffset)+visibleWidth>w||uint64_t(yOffset)+visibleHeight>h)throw Error("decoded visible resolution mismatch",E_FAIL);
  UINT32 rawStride=0;if(FAILED(type->GetUINT32(MF_MT_DEFAULT_STRIDE,&rawStride)))rawStride=w;stride=rawStride;
  LONGLONG pts=0;Check(sample->GetSampleTime(&pts),"decoded PTS");if(pts<=previous)throw Error("decoded PTS order",E_FAIL);previous=pts;
  ComPtr<IMFMediaBuffer> b;Check(sample->ConvertToContiguousBuffer(&b),"decoded buffer");BYTE* data=nullptr;DWORD length=0;Check(b->Lock(&data,nullptr,&length),"decoded CPU buffer (offline only)");
  UINT nonce=0,counter=0,ambiguous=0;BYTE minWhite=255,maxBlack=0;
  if(stride<w||uint64_t(stride)*h>length){b->Unlock();throw Error("decoded buffer bounds",E_FAIL);}
  for(UINT row=0;row<2;++row){UINT value=0;UINT y=yOffset+(row?96:48)*height/1080;
   for(UINT bit=0;bit<32;++bit){UINT x=xOffset+(32+bit*20+8)*width/2400;BYTE v=data[size_t(y)*stride+x];if(v>=175){value|=1u<<bit;minWhite=(std::min)(minWhite,v);}else if(v<=80)maxBlack=(std::max)(maxBlack,v);else ++ambiguous;}
   if(row==0)nonce=value;else counter=value;
  }
  Check(b->Unlock(),"decoded buffer unlock");if(ambiguous)throw Error("ambiguous decoded diagnostic cell",E_FAIL);
  fprintf(csv.get(),"%llu,%lld,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",count,pts,width,height,nonce,counter,ambiguous,minWhite,maxBlack,w,h,xOffset,yOffset);++count;
 }
public:
 Decoder(UINT w,UINT h,const std::wstring& directory):width(w),height(h){
  csv=Open(directory+L"/decoded-frames.csv",L"w");fprintf(csv.get(),"index,pts,width,height,nonce,counter,ambiguous,min_white,max_black,coded_width,coded_height,aperture_x,aperture_y\n");
  Check(CoCreateInstance(CLSID_CMSH264DecoderMFT,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&mft)),"Microsoft independent H264 decoder");
  ComPtr<IMFMediaType> input;Check(MFCreateMediaType(&input),"decoder input type");Check(input->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video),"major");Check(input->SetGUID(MF_MT_SUBTYPE,MFVideoFormat_H264),"H264");Check(mft->SetInputType(0,input.Get(),0),"decoder SetInput");
  SelectOutput();Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING,0),"decoder begin");Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM,0),"decoder start");
 }
 void Pull(){
  for(unsigned attempt=0;attempt<256;++attempt){
   MFT_OUTPUT_STREAM_INFO info{};Check(mft->GetOutputStreamInfo(0,&info),"decoder output info");ComPtr<IMFSample> provided;
   if(!(info.dwFlags&MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)){Check(MFCreateSample(&provided),"decoded sample");ComPtr<IMFMediaBuffer> b;Check(MFCreateMemoryBuffer((std::max)(info.cbSize,DWORD(width*height*2)),&b),"decoded output storage");Check(provided->AddBuffer(b.Get()),"decoded storage attach");}
   MFT_OUTPUT_DATA_BUFFER out{};out.pSample=provided.Get();DWORD status=0;HRESULT hr=mft->ProcessOutput(0,1,&out,&status);if(out.pEvents)out.pEvents->Release();ComPtr<IMFSample> sample;if(out.pSample&&out.pSample!=provided.Get())sample.Attach(out.pSample);else sample=provided;
   if(hr==MF_E_TRANSFORM_STREAM_CHANGE){SelectOutput();continue;}if(hr==MF_E_TRANSFORM_NEED_MORE_INPUT)return;Check(hr,"decoder ProcessOutput");if(!sample)throw Error("decoder empty output",E_FAIL);Pixels(sample.Get());
  }
  throw Error("decoder bounded output loop exceeded",E_FAIL);
 }
 void Push(const std::vector<BYTE>& data,LONGLONG pts){
  ComPtr<IMFSample> s;ComPtr<IMFMediaBuffer> b;Check(MFCreateSample(&s),"AU sample");Check(MFCreateMemoryBuffer(DWORD(data.size()),&b),"AU buffer");BYTE* bytes=nullptr;Check(b->Lock(&bytes,nullptr,nullptr),"AU lock");memcpy(bytes,data.data(),data.size());Check(b->Unlock(),"AU unlock");Check(b->SetCurrentLength(DWORD(data.size())),"AU length");Check(s->AddBuffer(b.Get()),"AU attach");Check(s->SetSampleTime(pts),"AU timestamp");
  HRESULT hr=mft->ProcessInput(0,s.Get(),0);if(hr==MF_E_NOTACCEPTING){Pull();hr=mft->ProcessInput(0,s.Get(),0);}Check(hr,"decoder ProcessInput");Pull();
 }
 uint64_t Finish(){Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM,0),"decoder EOS");Check(mft->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN,0),"decoder drain");Pull();Check(mft->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING,0),"decoder end streaming");return count;}
};
int wmain(int argc,wchar_t** argv){
 if(argc!=4&&argc!=5)return 2;HRESULT co=CoInitializeEx(nullptr,COINIT_MULTITHREADED);if(FAILED(co))return 1;HRESULT startup=MFStartup(MF_VERSION);if(FAILED(startup)){CoUninitialize();return 1;}
 int result=0;
 try{
  std::wstring dir=argv[1];UINT width=wcstoul(argv[2],nullptr,10),height=wcstoul(argv[3],nullptr,10);if(width<800||width>2400||height<360||height>1080)throw Error("invalid decoder geometry",E_INVALIDARG);
  std::wstring output=argc==5?argv[4]:dir;
  auto source=Open(dir+L"/access-units.bin",L"rb");Decoder decoder(width,height,output);uint64_t inputs=0,total=0;LONGLONG previous=-1;
  for(;;){DWORD length=0;size_t got=fread(&length,1,sizeof(length),source.get());if(!got&&feof(source.get()))break;if(got!=sizeof(length)||!length||length>16*1024*1024)throw Error("invalid AU record length",E_INVALIDARG);LONGLONG pts=0;if(fread(&pts,sizeof(pts),1,source.get())!=1||pts<=previous)throw Error("invalid AU timestamp",E_INVALIDARG);previous=pts;std::vector<BYTE> data(length);if(fread(data.data(),1,length,source.get())!=length)throw Error("truncated AU",E_INVALIDARG);decoder.Push(data,pts);++inputs;total+=length;}
  uint64_t decoded=decoder.Finish();if(inputs<2||decoded!=inputs)throw Error("decode exact frame count",E_FAIL);
  auto report=Open(output+L"/decode-result.json",L"w");fprintf(report.get(),"{\"outcome\":\"PASS_DECODE\",\"width\":%u,\"height\":%u,\"inputs\":%llu,\"decoded\":%llu,\"bytes\":%llu,\"decoder\":\"CLSID_CMSH264DecoderMFT\",\"offline_cpu_decode\":true,\"normal_encoder_readback\":false,\"content_comparison_requires_verifier\":true}\n",width,height,inputs,decoded,total);printf("PASS_DECODE inputs=%llu decoded=%llu bytes=%llu; content comparison still requires verifier\n",inputs,decoded,total);
 }catch(const Error& e){fprintf(stderr,"DECODE_ERROR %s HRESULT=0x%08lX\n",e.what(),ULONG(e.hr));result=1;}catch(const std::exception& e){fprintf(stderr,"DECODE_ERROR %s\n",e.what());result=1;}
 MFShutdown();CoUninitialize();return result;
}
