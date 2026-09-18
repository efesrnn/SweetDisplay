// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include <mfapi.h>
#include <mfidl.h>
#include <mferror.h>
#include <mftransform.h>
#include <codecapi.h>
#include <strmif.h>
#include <d3d10.h>
#include <array>
#include <map>
#include <algorithm>

namespace SweetDisplay::Encoding {
constexpr unsigned PoolSize=4;
struct Lifetime {
 bool com=false,mf=false;
 Lifetime(){Hr(CoInitializeEx(nullptr,COINIT_MULTITHREADED),"encode CoInitializeEx");com=true;try{Hr(MFStartup(MF_VERSION),"MFStartup");mf=true;}catch(...){CoUninitialize();com=false;throw;}}
 ~Lifetime(){if(mf)MFShutdown();if(com)CoUninitialize();}
};
struct PoolState {std::array<std::atomic<bool>,PoolSize> busy{};};
class Returned final:public Microsoft::WRL::RuntimeClass<Microsoft::WRL::RuntimeClassFlags<Microsoft::WRL::ClassicCom>,IMFAsyncCallback> {
public:
 std::shared_ptr<PoolState> state;unsigned slot=0;
 STDMETHODIMP GetParameters(DWORD*,DWORD*) override{return E_NOTIMPL;}
 STDMETHODIMP Invoke(IMFAsyncResult*) override{state->busy[slot].store(false);return S_OK;}
};
struct Settings {UINT width=800,height=360,fps=10,bitrate=2000000;bool uncappedSubmission=false;};
class Encoder {
 Lifetime lifetime;
 ComPtr<IMFActivate> activation;ComPtr<IMFTransform> transform;
 ComPtr<IMFMediaEventGenerator> events;ComPtr<IMFDXGIDeviceManager> manager;
 ComPtr<ID3D11Device> device;ComPtr<ID3D11DeviceContext> context;
 ComPtr<ID3D11VideoDevice> video;ComPtr<ID3D11VideoContext> videoContext;
 ComPtr<ID3D11VideoProcessorEnumerator> enumerator;ComPtr<ID3D11VideoProcessor> processor;
 std::array<ComPtr<ID3D11Texture2D>,PoolSize> surfaces;
 std::array<ComPtr<ID3D11VideoProcessorOutputView>,PoolSize> views;
 std::array<ComPtr<Returned>,PoolSize> returned;
 std::shared_ptr<PoolState> pool=std::make_shared<PoolState>();
 ComPtr<ID3D11Query> completion;
 File calls,inputLog,outputLog,stream,units;
 Settings settings;std::wstring directory;
 uint64_t frequency=0,initialized=0,firstSource=0,nextDue=0,firstAcceptedQpc=0,lastOutputQpc=0;
 DWORD inputId=0,outputId=0;UINT tokens=0;bool drained=false,shutdown=false;
 struct Pending {uint64_t id,source,submitted;UINT nonce,counter;};
 std::map<LONGLONG,Pending> pending;
 uint64_t seen=0,accepted=0,rateDrops=0,pressureDrops=0,bytes=0,outputs=0,keyframes=0,idrs=0;
 uint64_t highWater=0,poolPeak=0;double firstLatency=0,latencySum=0,latencyPeak=0,conversionSum=0,conversionPeak=0;
 std::array<uint64_t,50001> latencyHistogram{}; // Fixed 0.1 ms bins; exact values stay in the evidence CSV.
 void Call(HRESULT hr,const char* name){fprintf(calls.get(),"%s=0x%08lX\n",name,ULONG(hr));fflush(calls.get());Hr(hr,name);}
 void Codec(ICodecAPI* codec,const GUID& key,ULONG value,bool boolean,const char* name){
  VARIANT v;VariantInit(&v);if(boolean){v.vt=VT_BOOL;v.boolVal=value?VARIANT_TRUE:VARIANT_FALSE;}else{v.vt=VT_UI4;v.ulVal=value;}
  HRESULT hr=codec->SetValue(&key,&v);fprintf(calls.get(),"codec %s requested=%lu set=0x%08lX\n",name,value,ULONG(hr));
  if(FAILED(hr))throw Error(name,DWORD(hr));
  VARIANT actual;VariantInit(&actual);hr=codec->GetValue(&key,&actual);fprintf(calls.get(),"codec %s get=0x%08lX type=%u value=%lu\n",name,ULONG(hr),actual.vt,actual.vt==VT_BOOL?ULONG(actual.boolVal!=0):actual.ulVal);VariantClear(&actual);fflush(calls.get());
 }
 ComPtr<IMFMediaType> Type(GUID subtype,bool output){ComPtr<IMFMediaType> t;Call(MFCreateMediaType(&t),"media type");Call(t->SetGUID(MF_MT_MAJOR_TYPE,MFMediaType_Video),"major");Call(t->SetGUID(MF_MT_SUBTYPE,subtype),"subtype");Call(MFSetAttributeSize(t.Get(),MF_MT_FRAME_SIZE,settings.width,settings.height),"size");Call(MFSetAttributeRatio(t.Get(),MF_MT_FRAME_RATE,settings.fps,1),"rate");Call(MFSetAttributeRatio(t.Get(),MF_MT_PIXEL_ASPECT_RATIO,1,1),"aspect");Call(t->SetUINT32(MF_MT_INTERLACE_MODE,MFVideoInterlace_Progressive),"progressive");if(output){Call(t->SetUINT32(MF_MT_AVG_BITRATE,settings.bitrate),"bitrate");Call(t->SetUINT32(MF_MT_MPEG2_PROFILE,eAVEncH264VProfile_Main),"main profile");}return t;}
 void Output(){
  MFT_OUTPUT_STREAM_INFO info{};CallQuiet(transform->GetOutputStreamInfo(outputId,&info),"output stream info");
  ComPtr<IMFSample> provided;
  if(!(info.dwFlags&MFT_OUTPUT_STREAM_PROVIDES_SAMPLES)){
   Hr(MFCreateSample(&provided),"output sample");ComPtr<IMFMediaBuffer> b;Hr(MFCreateMemoryBuffer((std::max)(info.cbSize,DWORD(settings.width*settings.height*2)),&b),"encoded buffer");Hr(provided->AddBuffer(b.Get()),"encoded buffer attach");
  }
  MFT_OUTPUT_DATA_BUFFER out{};out.dwStreamID=outputId;out.pSample=provided.Get();DWORD status=0;
  HRESULT hr=transform->ProcessOutput(0,1,&out,&status);if(out.pEvents)out.pEvents->Release();
  ComPtr<IMFSample> sample;if(out.pSample&&out.pSample!=provided.Get())sample.Attach(out.pSample);else sample=provided;
  CallQuiet(hr,"ProcessOutput");if(!sample)throw Error("empty encoded sample",ERROR_INVALID_DATA);
  LONGLONG pts=0;Hr(sample->GetSampleTime(&pts),"encoded timestamp");auto it=pending.find(pts);if(it==pending.end())throw Error("encoded frame identity mismatch",ERROR_INVALID_DATA);
  ComPtr<IMFMediaBuffer> buffer;Hr(sample->ConvertToContiguousBuffer(&buffer),"encoded bytes");BYTE* data=nullptr;DWORD length=0;Hr(buffer->Lock(&data,nullptr,&length),"lock encoded bytes");
  bool idr=false,sps=false,pps=false,vcl=false;
  for(DWORD i=0;i+4<length;++i){unsigned offset=0;if(data[i]==0&&data[i+1]==0&&data[i+2]==1)offset=3;else if(data[i]==0&&data[i+1]==0&&data[i+2]==0&&data[i+3]==1)offset=4;if(offset){auto type=data[i+offset]&31;idr|=type==5;sps|=type==7;pps|=type==8;vcl|=type==1||type==5;i+=offset-1;}}
  const bool wrote=length&&fwrite(data,1,length,stream.get())==length&&fwrite(&length,sizeof(length),1,units.get())==1&&fwrite(&pts,sizeof(pts),1,units.get())==1&&fwrite(data,1,length,units.get())==length;
  Hr(buffer->Unlock(),"unlock encoded bytes");if(!wrote||!vcl)throw Error("invalid/non-VCL H264 output or write failure",ERROR_INVALID_DATA);
  UINT32 clean=0;sample->GetUINT32(MFSampleExtension_CleanPoint,&clean);
  const uint64_t now=Qpc();const double ms=Milliseconds(now-it->second.submitted,frequency);
  if(!outputs)firstLatency=Milliseconds(now-initialized,frequency);
  ++latencyHistogram[(std::min)(size_t(50000),size_t(ms*10))];latencySum+=ms;latencyPeak=(std::max)(latencyPeak,ms);bytes+=length;++outputs;keyframes+=clean!=0;idrs+=idr;lastOutputQpc=now;
  fprintf(outputLog.get(),"%llu,%lld,%llu,%llu,%llu,%lu,%u,%u,%u,%u,%.6f\n",it->second.id,pts,it->second.source,it->second.submitted,now,length,clean,idr,sps,pps,ms);
  pending.erase(it);
 }
 void CallQuiet(HRESULT hr,const char* name){if(hr!=S_OK)Call(hr,name);}
public:
 Encoder(ID3D11Device* d,ID3D11DeviceContext* c,LUID adapter,const Settings& s,const std::wstring& dir,uint64_t freq):device(d),context(c),settings(s),directory(dir),frequency(freq){
  initialized=Qpc();calls=Open(dir+L"/encoder-calls.txt");inputLog=Open(dir+L"/encode-input.csv");outputLog=Open(dir+L"/encode-output.csv");
  FILE* f=nullptr;if(_wfopen_s(&f,(dir+L"/stream.h264").c_str(),L"wb"))throw Error("open H264 evidence",ERROR_OPEN_FAILED);stream.reset(f);
  if(_wfopen_s(&f,(dir+L"/access-units.bin").c_str(),L"wb"))throw Error("open AU evidence",ERROR_OPEN_FAILED);units.reset(f);
  fprintf(inputLog.get(),"frame_id,source_qpc,nonce,counter,decision,pts,submit_qpc,conversion_ms,pending,pool_busy\n");
  fprintf(outputLog.get(),"frame_id,pts,source_qpc,submit_qpc,output_qpc,bytes,clean_point,idr,sps,pps,latency_ms\n");
  fprintf(calls.get(),"GPU_native=1 software_fallback=0 adapter=%08lx:%08lx format=NV12 width=%u height=%u target_fps=%u bitrate=%u pool=%u\n",ULONG(adapter.HighPart),adapter.LowPart,s.width,s.height,s.fps,s.bitrate,PoolSize);
  fprintf(calls.get(),"submission_rate_limiter=%s\n",s.uncappedSubmission?"disabled":"enabled");
  ComPtr<ID3D10Multithread> mt;Call(device.As(&mt),"D3D multithread");mt->SetMultithreadProtected(TRUE);
  ComPtr<IMFAttributes> filter;Call(MFCreateAttributes(&filter,1),"filter");Call(filter->SetBlob(MFT_ENUM_ADAPTER_LUID,reinterpret_cast<BYTE*>(&adapter),sizeof(adapter)),"adapter LUID");
  MFT_REGISTER_TYPE_INFO output{MFMediaType_Video,MFVideoFormat_H264};IMFActivate** list=nullptr;UINT32 count=0;
  Call(MFTEnum2(MFT_CATEGORY_VIDEO_ENCODER,MFT_ENUM_FLAG_HARDWARE|MFT_ENUM_FLAG_SORTANDFILTER,nullptr,&output,filter.Get(),&list,&count),"MFTEnum2 hardware only");
  if(!count)throw Error("no same-adapter hardware encoder",DWORD(MF_E_TOPO_CODEC_NOT_FOUND));
  activation=list[0];for(UINT32 i=0;i<count;++i)list[i]->Release();CoTaskMemFree(list);
  wchar_t* name=nullptr;UINT32 chars=0;Call(activation->GetAllocatedString(MFT_FRIENDLY_NAME_Attribute,&name,&chars),"encoder name");fprintf(calls.get(),"selected_name=%ls\n",name);CoTaskMemFree(name);
  GUID id{};Call(activation->GetGUID(MFT_TRANSFORM_CLSID_Attribute,&id),"encoder CLSID");wchar_t guid[40]{};StringFromGUID2(id,guid,40);fprintf(calls.get(),"selected_clsid=%ls\n",guid);
  Call(activation->GetAllocatedString(MFT_ENUM_HARDWARE_URL_Attribute,&name,&chars),"hardware URL");fprintf(calls.get(),"hardware_url=%ls\n",name);CoTaskMemFree(name);
  Call(activation->ActivateObject(IID_PPV_ARGS(&transform)),"Activate hardware MFT");ComPtr<IMFAttributes> attrs;Call(transform->GetAttributes(&attrs),"MFT attributes");UINT32 async=0,aware=0;Call(attrs->GetUINT32(MF_TRANSFORM_ASYNC,&async),"MF_TRANSFORM_ASYNC");Call(attrs->GetUINT32(MF_SA_D3D11_AWARE,&aware),"MF_SA_D3D11_AWARE");if(!async||!aware)throw Error("hardware MFT lacks async/D3D11 support",DWORD(E_NOINTERFACE));fprintf(calls.get(),"async=1 d3d11_aware=1\n");
  Call(attrs->SetUINT32(MF_TRANSFORM_ASYNC_UNLOCK,TRUE),"unlock async");Call(transform.As(&events),"event generator");
  UINT reset=0;Call(MFCreateDXGIDeviceManager(&reset,&manager),"MFCreateDXGIDeviceManager");Call(manager->ResetDevice(d,reset),"DXGI ResetDevice");Call(transform->ProcessMessage(MFT_MESSAGE_SET_D3D_MANAGER,reinterpret_cast<ULONG_PTR>(manager.Get())),"MFT SET_D3D_MANAGER");
  DWORD ins=0,outs=0;Call(transform->GetStreamCount(&ins,&outs),"stream count");if(ins!=1||outs!=1)throw Error("expected one encoder stream",DWORD(E_NOTIMPL));HRESULT ids=transform->GetStreamIDs(1,&inputId,1,&outputId);if(ids!=E_NOTIMPL)Call(ids,"stream IDs");
  ComPtr<ICodecAPI> codec;Call(transform.As(&codec),"ICodecAPI");Codec(codec.Get(),CODECAPI_AVLowLatencyMode,1,true,"low latency");Codec(codec.Get(),CODECAPI_AVEncMPVDefaultBPictureCount,0,false,"B frames");Codec(codec.Get(),CODECAPI_AVEncMPVGOPSize,s.fps*2,false,"GOP");Codec(codec.Get(),CODECAPI_AVEncCommonRateControlMode,eAVEncCommonRateControlMode_CBR,false,"CBR");
  auto out=Type(MFVideoFormat_H264,true);Call(transform->SetOutputType(outputId,out.Get(),0),"SetOutput H264");auto in=Type(MFVideoFormat_NV12,false);Call(transform->SetInputType(inputId,in.Get(),0),"SetInput NV12");
  Call(device.As(&video),"D3D11 video device");Call(context.As(&videoContext),"D3D11 video context");
  D3D11_VIDEO_PROCESSOR_CONTENT_DESC content{};content.InputFrameFormat=D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE;content.InputWidth=2400;content.InputHeight=1080;content.OutputWidth=s.width;content.OutputHeight=s.height;content.InputFrameRate={60,1};content.OutputFrameRate={s.fps,1};content.Usage=D3D11_VIDEO_USAGE_PLAYBACK_NORMAL;
  Call(video->CreateVideoProcessorEnumerator(&content,&enumerator),"video processor enumerator");UINT flags=0;Call(enumerator->CheckVideoProcessorFormat(DXGI_FORMAT_B8G8R8A8_UNORM,&flags),"BGRA support");if(!(flags&D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_INPUT))throw Error("BGRA input unsupported",DWORD(E_NOTIMPL));Call(enumerator->CheckVideoProcessorFormat(DXGI_FORMAT_NV12,&flags),"NV12 support");if(!(flags&D3D11_VIDEO_PROCESSOR_FORMAT_SUPPORT_OUTPUT))throw Error("NV12 output unsupported",DWORD(E_NOTIMPL));
  Call(video->CreateVideoProcessor(enumerator.Get(),0,&processor),"video processor");
  D3D11_VIDEO_PROCESSOR_COLOR_SPACE rgb{};rgb.RGB_Range=0;rgb.Nominal_Range=D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_0_255;
  D3D11_VIDEO_PROCESSOR_COLOR_SPACE yuv{};yuv.YCbCr_Matrix=1;yuv.Nominal_Range=D3D11_VIDEO_PROCESSOR_NOMINAL_RANGE_16_235;
  videoContext->VideoProcessorSetStreamColorSpace(processor.Get(),0,&rgb);videoContext->VideoProcessorSetOutputColorSpace(processor.Get(),&yuv);
  RECT src{0,0,2400,1080},dst{0,0,LONG(s.width),LONG(s.height)};videoContext->VideoProcessorSetStreamSourceRect(processor.Get(),0,TRUE,&src);videoContext->VideoProcessorSetStreamDestRect(processor.Get(),0,TRUE,&dst);videoContext->VideoProcessorSetOutputTargetRect(processor.Get(),TRUE,&dst);videoContext->VideoProcessorSetStreamFrameFormat(processor.Get(),0,D3D11_VIDEO_FRAME_FORMAT_PROGRESSIVE);videoContext->VideoProcessorSetStreamAutoProcessingMode(processor.Get(),0,FALSE);
  for(unsigned i=0;i<PoolSize;++i){D3D11_TEXTURE2D_DESC desc{};desc.Width=s.width;desc.Height=s.height;desc.MipLevels=1;desc.ArraySize=1;desc.Format=DXGI_FORMAT_NV12;desc.SampleDesc.Count=1;desc.Usage=D3D11_USAGE_DEFAULT;desc.BindFlags=D3D11_BIND_RENDER_TARGET;Call(d->CreateTexture2D(&desc,nullptr,&surfaces[i]),"NV12 GPU surface");D3D11_VIDEO_PROCESSOR_OUTPUT_VIEW_DESC ov{};ov.ViewDimension=D3D11_VPOV_DIMENSION_TEXTURE2D;Call(video->CreateVideoProcessorOutputView(surfaces[i].Get(),enumerator.Get(),&ov,&views[i]),"NV12 output view");returned[i]=Microsoft::WRL::Make<Returned>();returned[i]->state=pool;returned[i]->slot=i;}
  D3D11_QUERY_DESC query{D3D11_QUERY_EVENT,0};Call(d->CreateQuery(&query,&completion),"conversion completion query");
  Call(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_BEGIN_STREAMING,0),"BEGIN_STREAMING");Call(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_START_OF_STREAM,0),"START_OF_STREAM");
 }
 ~Encoder(){if(transform&&!shutdown){ComPtr<IMFShutdown> s;if(SUCCEEDED(transform.As(&s)))s->Shutdown();}}
 void Pump(){
  for(unsigned n=0;n<64;++n){ComPtr<IMFMediaEvent> e;HRESULT hr=events->GetEvent(MF_EVENT_FLAG_NO_WAIT,&e);if(hr==MF_E_NO_EVENTS_AVAILABLE)return;CallQuiet(hr,"MFT GetEvent");HRESULT status=S_OK;Hr(e->GetStatus(&status),"event status");CallQuiet(status,"MFT event failure");MediaEventType type=MEUnknown;Hr(e->GetType(&type),"event type");if(type==METransformNeedInput){if(++tokens>64)throw Error("unbounded MFT input demand",ERROR_INVALID_DATA);}else if(type==METransformHaveOutput)Output();else if(type==METransformDrainComplete)drained=true;else if(type==MEError)throw Error("MFT error event",DWORD(E_FAIL));}
 }
 void Submit(ID3D11Texture2D* texture,const Frame& frame,UINT nonce,UINT counter){
  Pump();++seen;if(!firstSource){firstSource=frame.qpc;nextDue=frame.qpc;}
  if(!settings.uncappedSubmission){
   if(frame.qpc<nextDue){++rateDrops;fprintf(inputLog.get(),"%llu,%llu,%u,%u,rate,0,0,0,%zu,0\n",frame.id,frame.qpc,nonce,counter,pending.size());return;}
   nextDue=firstSource+(((frame.qpc-firstSource)*settings.fps/frequency)+1)*frequency/settings.fps;
  }
  int slot=-1;unsigned busy=0;for(unsigned i=0;i<PoolSize;++i){if(pool->busy[i].load())++busy;else if(slot<0)slot=int(i);}
  if(!tokens||slot<0||pending.size()>=PoolSize){++pressureDrops;fprintf(inputLog.get(),"%llu,%llu,%u,%u,backpressure,0,0,0,%zu,%u\n",frame.id,frame.qpc,nonce,counter,pending.size(),busy);return;}
  const uint64_t begin=Qpc();D3D11_VIDEO_PROCESSOR_INPUT_VIEW_DESC iv{};iv.ViewDimension=D3D11_VPIV_DIMENSION_TEXTURE2D;
  ComPtr<ID3D11VideoProcessorInputView> view;Hr(video->CreateVideoProcessorInputView(texture,enumerator.Get(),&iv,&view),"BGRA processor view");
  D3D11_VIDEO_PROCESSOR_STREAM input{};input.Enable=TRUE;input.pInputSurface=view.Get();Hr(videoContext->VideoProcessorBlt(processor.Get(),views[slot].Get(),0,1,&input),"GPU BGRA-NV12 conversion");context->End(completion.Get());context->Flush();
  BOOL done=FALSE;while(!done){HRESULT hr=context->GetData(completion.Get(),&done,sizeof(done),0);if(FAILED(hr))Call(hr,"conversion GPU completion");if(Qpc()-begin>frequency/2)throw Error("GPU conversion timeout",WAIT_TIMEOUT);if(!done)Sleep(0);}
  double conversion=Milliseconds(Qpc()-begin,frequency);conversionSum+=conversion;conversionPeak=(std::max)(conversionPeak,conversion);
  ComPtr<IMFTrackedSample> tracked;Hr(MFCreateTrackedSample(&tracked),"tracked GPU sample");ComPtr<IMFSample> sample;Hr(tracked.As(&sample),"tracked sample interface");ComPtr<IMFMediaBuffer> buffer;Hr(MFCreateDXGISurfaceBuffer(__uuidof(ID3D11Texture2D),surfaces[slot].Get(),0,FALSE,&buffer),"GPU-backed input buffer");
  ComPtr<IMF2DBuffer> twoD;Hr(buffer.As(&twoD),"GPU buffer 2D interface");DWORD contiguousLength=0;Hr(twoD->GetContiguousLength(&contiguousLength),"GPU buffer length");Hr(buffer->SetCurrentLength(contiguousLength),"GPU buffer current length");
  ComPtr<IMFDXGIBuffer> dxgiBuffer;Hr(buffer.As(&dxgiBuffer),"input IMFDXGIBuffer");ComPtr<ID3D11Texture2D> resource;Hr(dxgiBuffer->GetResource(IID_PPV_ARGS(&resource)),"input GPU resource identity");
  if(resource.Get()!=surfaces[slot].Get())throw Error("input GPU surface identity mismatch",ERROR_INVALID_DATA);
  if(!accepted){D3D11_TEXTURE2D_DESC proof{};resource->GetDesc(&proof);fprintf(calls.get(),"input_surface_proof format=%u width=%u height=%u usage=%u cpu_access=%u current_length=%lu\n",proof.Format,proof.Width,proof.Height,proof.Usage,proof.CPUAccessFlags,contiguousLength);fflush(calls.get());}
  Hr(sample->AddBuffer(buffer.Get()),"GPU buffer attachment");
  LONGLONG pts=LONGLONG((frame.qpc-firstSource)*10000000/frequency);Hr(sample->SetSampleTime(pts),"input timestamp");Hr(sample->SetSampleDuration(10000000/settings.fps),"input duration");
  pool->busy[slot].store(true);HRESULT allocator=tracked->SetAllocator(returned[slot].Get(),nullptr);if(allocator!=S_OK){pool->busy[slot].store(false);Call(allocator,"tracked allocator");}
  uint64_t submitted=Qpc();CallQuiet(transform->ProcessInput(inputId,sample.Get(),0),"ProcessInput GPU sample");--tokens;
  if(!pending.emplace(pts,Pending{frame.id,frame.qpc,submitted,nonce,counter}).second)throw Error("duplicate encode timestamp",ERROR_INVALID_DATA);
  if(!accepted)firstAcceptedQpc=submitted;++accepted;highWater=(std::max)(highWater,uint64_t(pending.size()));poolPeak=(std::max)(poolPeak,uint64_t(busy+1));
  fprintf(inputLog.get(),"%llu,%llu,%u,%u,accepted,%lld,%llu,%.6f,%zu,%u\n",frame.id,frame.qpc,nonce,counter,pts,submitted,conversion,pending.size(),busy+1);
 }
 void Finish(double inputSeconds){
  Call(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_OF_STREAM,inputId),"END_OF_STREAM");Call(transform->ProcessMessage(MFT_MESSAGE_COMMAND_DRAIN,0),"COMMAND_DRAIN");
  const auto deadline=Qpc()+frequency*5;while(!drained){Pump();if(Qpc()>deadline)throw Error("encoder drain timeout",WAIT_TIMEOUT);Sleep(1);}
  if(!pending.empty()||accepted!=outputs||seen!=accepted+rateDrops+pressureDrops||!outputs)throw Error("encode exact accounting failure",ERROR_INVALID_DATA);
  Call(transform->ProcessMessage(MFT_MESSAGE_NOTIFY_END_STREAMING,0),"END_STREAMING");ComPtr<IMFShutdown> s;Call(transform.As(&s),"IMFShutdown");Call(s->Shutdown(),"encoder Shutdown");shutdown=true;events.Reset();transform.Reset();activation.Reset();
  const auto released=Qpc()+frequency*2;for(;;){bool busy=false;for(auto& b:pool->busy)busy|=b.load();if(!busy)break;if(Qpc()>released)throw Error("GPU sample lease not released after shutdown",WAIT_TIMEOUT);Sleep(1);}
  auto p=[&](double q){uint64_t count=0,target=uint64_t(q*outputs+.999999);for(size_t i=0;i<latencyHistogram.size();++i){count+=latencyHistogram[i];if(count>=target)return i*.1;}return 5000.;};
  auto report=Open(directory+L"/encode-result.json");
  fprintf(report.get(),"{\"width\":%u,\"height\":%u,\"target_fps\":%u,\"target_bitrate\":%u,\"seconds\":%.9f,\"input\":%llu,\"accepted\":%llu,\"outputs\":%llu,\"rate_drops\":%llu,\"backpressure_drops\":%llu,\"input_fps\":%.6f,\"output_fps\":%.6f,\"bytes\":%llu,\"bitrate\":%.6f,\"startup_ms\":%.6f,\"latency_avg_ms\":%.6f,\"latency_peak_ms\":%.6f,\"latency_p50_ms\":%.6f,\"latency_p95_ms\":%.6f,\"latency_p99_ms\":%.6f,\"conversion_avg_ms\":%.6f,\"conversion_peak_ms\":%.6f,\"queue_peak\":%llu,\"pool_peak\":%llu,\"keyframes\":%llu,\"idr\":%llu,\"invalid\":0,\"pending\":0,\"hardware_only\":true,\"gpu_surface_input\":true,\"software_fallback\":false,\"drained\":true,\"clean_shutdown\":true}\n",settings.width,settings.height,settings.fps,settings.bitrate,inputSeconds,seen,accepted,outputs,rateDrops,pressureDrops,seen/inputSeconds,outputs/inputSeconds,bytes,bytes*8.0/inputSeconds,firstLatency,latencySum/outputs,latencyPeak,p(.5),p(.95),p(.99),conversionSum/accepted,conversionPeak,highWater,poolPeak,keyframes,idrs);
  printf("ENCODE complete input=%llu accepted=%llu output=%llu rate_drop=%llu pressure_drop=%llu peak=%llu latency_avg=%.3fms\n",seen,accepted,outputs,rateDrops,pressureDrops,highWater,latencySum/outputs);
 }
};
}
