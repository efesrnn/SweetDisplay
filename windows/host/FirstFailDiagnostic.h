// Opt-in, bounded first-failure evidence. Never used by the normal video path.
#pragma once
#include "../shared/PatternOracle.h"
#include "../shared/DesktopObservation.h"
#include "../shared/FrameHandoffProtocol.h"
#include <d3d11.h>
#include <wrl.h>
#include <deque>
#include <filesystem>
#include <vector>
#include <exception>
#include "DesktopSample.h"
#include "../shared/ContentClassification.h"
namespace SweetDisplay::FirstFail {
using Microsoft::WRL::ComPtr;
struct Record {Handoff::State state{};uint64_t receive=0,acquired=0,sampled=0,hash=0,texture=0;uint32_t nonce=0,counter=0,previous=0,published=0,reason=0;uint64_t renderQpc=0;};
inline void PrivateDirectory(const std::wstring& output){
 wchar_t exe[32768]{};if(!GetModuleFileNameW(nullptr,exe,32768))throw std::runtime_error("Cannot identify private diagnostic root");
 auto root=std::filesystem::weakly_canonical(std::filesystem::path(exe).parent_path().parent_path().parent_path()/L"docs/evidence/private").wstring()+L"\\";
 auto dir=std::filesystem::weakly_canonical(output).wstring()+L"\\";
 if(dir.size()<=root.size()||_wcsnicmp(dir.c_str(),root.c_str(),root.size())!=0)throw std::runtime_error("First-fail evidence must be inside repository ignored docs/evidence/private");
}
class Diagnostic {
 PatternOracle::Mapping producer;std::deque<Record> history;std::wstring output;uint32_t expected=0,previous=0;bool frozen=false;
 bool classified=false;FILE* classifications=nullptr;uint64_t counts[6]{},epoch=0,generation=0,identities[Handoff::Capacity]{},lastFlush=0;
 FILE* File(const wchar_t* name){FILE* f=nullptr;if(_wfopen_s(&f,(output+L"/"+name).c_str(),L"w")||!f)throw std::runtime_error("Cannot write first-fail context");return f;}
public:
 ~Diagnostic(){if(classifications){if(std::uncaught_exceptions()&&!counts[Content::D]&&!counts[Content::E])++counts[Content::D];fclose(classifications);FILE* f=nullptr;if(!_wfopen_s(&f,(output+L"/classification-result.json").c_str(),L"w")&&f){fprintf(f,"{\"version\":1,\"A\":%llu,\"B\":%llu,\"C\":%llu,\"D\":%llu,\"E\":%llu}\n",counts[1],counts[2],counts[3],counts[4],counts[5]);fclose(f);}}}
 void Init(const std::wstring& dir,uint32_t nonce,const Handoff::Connect& config,ID3D11Texture2D* const* textures,bool corrected=false){
  PrivateDirectory(dir);output=dir;expected=nonce;
  if(std::filesystem::exists(output+L"/first-failure.json")||std::filesystem::exists(output+L"/first-failure-crop.bmp"))throw std::runtime_error("Refusing to overwrite first-fail evidence");
  producer.Open(nonce,false);
  classified=corrected;epoch=config.epoch;generation=producer.data->generation;
  for(uint32_t i=0;i<Handoff::Capacity;++i)identities[i]=uint64_t(textures[i]);
  if(classified){classifications=File(L"classifications.csv");fprintf(classifications,"frame_id,source_qpc,class,reason,nonce,counter,previous_counter,published,render_qpc,rgb_hash,reference_begin_qpc,reference_end_qpc,reference_hash1,reference_hash2,reference_error,reference_x,reference_y,reference_match\n");}
  FILE* f=File(L"resource-generation.txt");
  fprintf(f,"driver_epoch=%llu pattern_generation=%llu pattern_pid=%u pattern_hwnd=%llu adapter=%08lx:%08lx\n",config.epoch,producer.data->generation,producer.data->pid,producer.data->hwnd,ULONG(config.adapter.HighPart),config.adapter.LowPart);
  for(uint32_t i=0;i<Handoff::Capacity;++i)fprintf(f,"slot=%u shared_resource=%ls host_texture=%p\n",i,config.names[i],textures[i]);
  fprintf(f,"Source acquisition ID equals handoff frame.id. Driver publishes this slot only after CopyResource/Flush/ReleaseSync(1). Physical source COM pointer is not exposed by the unchanged ABI.\n");fclose(f);
 }
 uint32_t Observe(Record& r,const DesktopSample::Pixels& pixels){
  r.previous=previous;r.published=PatternOracle::Load(&producer.data->published);
  const bool found=producer.Lookup(r.counter,r.renderQpc);
  r.reason=PatternOracle::Evaluate(expected,r.nonce,previous,r.counter,r.published,found,r.renderQpc,r.state.frame.qpc);
  uint32_t stop=r.reason;
  if(classified){
   const auto& frame=r.state.frame;
   const bool integrity=frame.epoch==epoch&&frame.slot<Handoff::Capacity&&r.texture==identities[frame.slot]&&producer.data->magic==PatternOracle::Magic&&producer.data->version==PatternOracle::Version&&producer.data->nonce==expected&&producer.data->generation==generation&&r.state.stats.held==1&&r.state.stats.highWater<=Handoff::Capacity;
   const bool pattern=r.nonce==expected&&r.counter&&found&&r.counter<=r.published&&DesktopSample::BinaryCells(pixels,r.nonce,r.counter);
   const bool possible=!(pattern&&r.renderQpc>frame.qpc);
   DesktopSample::Result reference{};
   if(integrity&&possible&&(!pattern||r.counter<previous))reference=DesktopSample::Compare(pixels);
   auto kind=Content::Classify(integrity&&possible,pattern,r.counter<previous,reference.matches);
   ++counts[kind];
   fprintf(classifications,"%llu,%llu,%u,%u,%u,%u,%u,%u,%llu,%llu,%llu,%llu,%llu,%llu,%lu,%ld,%ld,%u\n",frame.id,frame.qpc,kind,r.reason,r.nonce,r.counter,previous,r.published,r.renderQpc,DesktopSample::RgbHash(pixels),reference.begin,reference.end,reference.hash1,reference.hash2,reference.error,reference.x,reference.y,reference.matches);
   if(kind==Content::D||kind==Content::E||r.sampled-lastFlush>=producer.data->frequency){fflush(classifications);lastFlush=r.sampled;}
   stop=(kind==Content::D||kind==Content::E)?uint32_t(kind):0;
   if(!stop&&pattern){previous=(std::max)(previous,r.counter);InterlockedExchange(&producer.data->hostCounter,LONG(r.counter));}
  }
  if(history.size()==120)history.pop_front();history.push_back(r);
  if(!classified&&!r.reason){previous=r.counter;InterlockedExchange(&producer.data->hostCounter,LONG(r.counter));}return stop;
 }
 void Freeze(const Record& r,ID3D11Device* device,ID3D11DeviceContext* context,ID3D11Texture2D* texture){
  if(frozen)throw std::runtime_error("Second diagnostic capture forbidden");frozen=true;
  constexpr UINT x=24,y=24,width=656,height=96;HRESULT capture=S_OK;uint64_t cropSampleHash=0;DWORD imageError=0;
  std::vector<BYTE> pixels;D3D11_TEXTURE2D_DESC desc{};texture->GetDesc(&desc);
  if(desc.Width< x+width||desc.Height< y+height||desc.Format!=DXGI_FORMAT_B8G8R8A8_UNORM)capture=E_INVALIDARG;
  ComPtr<ID3D11Texture2D> crop;
  if(SUCCEEDED(capture)){desc.Width=width;desc.Height=height;desc.Usage=D3D11_USAGE_STAGING;desc.BindFlags=desc.MiscFlags=0;desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;capture=device->CreateTexture2D(&desc,nullptr,&crop);}
  if(SUCCEEDED(capture)){
   D3D11_BOX box{x,y,0,x+width,y+height,1};context->CopySubresourceRegion(crop.Get(),0,0,0,0,texture,0,&box);
   D3D11_MAPPED_SUBRESOURCE map{};capture=context->Map(crop.Get(),0,D3D11_MAP_READ,0,&map);
   if(SUCCEEDED(capture)){pixels.resize(width*height*4);for(UINT row=0;row<height;++row)memcpy(pixels.data()+row*width*4,static_cast<const BYTE*>(map.pData)+row*map.RowPitch,width*4);context->Unmap(crop.Get(),0);
    cropSampleHash=1469598103934665603ull;for(UINT row:{24u,72u})for(UINT b=0;b<640*4;++b){cropSampleHash^=pixels[(row*width+8)*4+b];cropSampleHash*=1099511628211ull;}
    HANDLE file=CreateFileW((output+L"/first-failure-crop.bmp").c_str(),GENERIC_WRITE,0,nullptr,CREATE_NEW,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE)imageError=GetLastError();else{
     BITMAPFILEHEADER bh{};bh.bfType=0x4d42;bh.bfOffBits=sizeof(bh)+sizeof(BITMAPINFOHEADER);bh.bfSize=bh.bfOffBits+DWORD(pixels.size());
     BITMAPINFOHEADER bi{};bi.biSize=sizeof(bi);bi.biWidth=width;bi.biHeight=-LONG(height);bi.biPlanes=1;bi.biBitCount=32;bi.biCompression=BI_RGB;
     DWORD n=0;BOOL ok=WriteFile(file,&bh,sizeof(bh),&n,nullptr)&&n==sizeof(bh);ok=ok&&WriteFile(file,&bi,sizeof(bi),&n,nullptr)&&n==sizeof(bi);ok=ok&&WriteFile(file,pixels.data(),DWORD(pixels.size()),&n,nullptr)&&n==pixels.size();if(!ok)imageError=GetLastError()?GetLastError():ERROR_WRITE_FAULT;CloseHandle(file);
    }
   }
  }
  FILE* f=File(L"first-failure.json");const auto& s=r.state;const auto& frame=s.frame;
  fprintf(f,"{\"outcome\":\"CONTENT_ORACLE_FAILURE\",\"reason_flags\":%u,\"frame_id\":%llu,\"source_acquisition_id\":%llu,\"presentation_id\":%llu,\"epoch\":%llu,\"slot\":%u,\"host_texture\":\"%p\",\"source_qpc\":%llu,\"receive_qpc\":%llu,\"mutex_acquired_qpc\":%llu,\"sample_complete_qpc\":%llu,\"expected_nonce\":%u,\"observed_nonce\":%u,\"previous_counter\":%u,\"observed_counter\":%u,\"producer_published_counter\":%u,\"producer_render_qpc\":%llu,\"pattern_generation\":%llu,\"pattern_pid\":%u,\"pattern_hwnd\":%llu,\"width\":%u,\"height\":%u,\"format\":%u,\"ready\":%u,\"held\":%u,\"high_water\":%u,\"source_frames\":%llu,\"producer_drops\":%llu,\"host_stale_drops\":%llu,\"busy\":%llu,\"invalid\":%llu,\"contention_total\":%llu,\"sample_hash\":%llu,\"crop_sample_hash\":%llu,\"same_held_texture_sample_matches_crop\":%s,\"capture_hresult\":%lu,\"image_error\":%lu,\"crop_x\":%u,\"crop_y\":%u,\"crop_width\":%u,\"crop_height\":%u,\"history_capacity\":120}\n",
   r.reason,frame.id,frame.id,frame.presentation,frame.epoch,frame.slot,reinterpret_cast<void*>(r.texture),frame.qpc,r.receive,r.acquired,r.sampled,expected,r.nonce,r.previous,r.counter,r.published,r.renderQpc,producer.data->generation,producer.data->pid,producer.data->hwnd,frame.width,frame.height,frame.format,s.stats.depth,s.stats.held,s.stats.highWater,s.stats.source,s.stats.producerDrops,s.stats.hostQueueDrops,s.stats.busy,s.stats.invalid,s.contention,r.hash,cropSampleHash,SUCCEEDED(capture)&&r.hash==cropSampleHash?"true":"false",ULONG(capture),imageError,x,y,width,height);fclose(f);
  f=File(L"first-failure-history.csv");fprintf(f,"frame_id,source_acquisition_id,presentation_id,epoch,slot,texture,source_qpc,receive_qpc,acquired_qpc,sample_qpc,nonce,previous_counter,counter,published,reason,ready,held,producer_drops,host_stale_drops,contention_total\n");
  for(const auto& h:history){const auto& t=h.state;fprintf(f,"%llu,%llu,%llu,%llu,%u,%p,%llu,%llu,%llu,%llu,%u,%u,%u,%u,%u,%u,%u,%llu,%llu,%llu\n",t.frame.id,t.frame.id,t.frame.presentation,t.frame.epoch,t.frame.slot,reinterpret_cast<void*>(h.texture),t.frame.qpc,h.receive,h.acquired,h.sampled,h.nonce,h.previous,h.counter,h.published,h.reason,t.stats.depth,t.stats.held,t.stats.producerDrops,t.stats.hostQueueDrops,t.contention);}fclose(f);
  f=File(L"first-failure-desktop.csv");Observation::Header(f);Observation::Write(f,HWND(producer.data->hwnd),"first_failure",r.reason);fclose(f);
  f=File(L"first-failure-display.txt");Observation::Display(f);fclose(f);
 }
};
}
