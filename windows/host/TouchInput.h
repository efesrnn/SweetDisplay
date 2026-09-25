// Copyright (c) SweetDisplay contributors. See repository LICENSE.md.
#pragma once
#include "../protocol/TouchProfile.h"
#include <windows.h>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <thread>
#include <share.h>
#include <cerrno>

namespace SweetDisplay::TouchInput {
namespace T=SweetDisplay::Touch;
namespace P=SweetDisplay::Protocol;
enum class Mode {Validate,Inject};
struct Target {LONG left=0,top=0;DWORD width=0,height=0;LUID adapter{};UINT32 source=0,target=0;uint64_t token=0;};

inline uint64_t Mix(uint64_t h,uint64_t value){for(unsigned i=0;i<8;++i){h^=uint8_t(value>>(i*8));h*=1099511628211ULL;}return h;}
inline Target Discover(){
 UINT32 pathsCount=0,modesCount=0;LONG code=GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&pathsCount,&modesCount);P::Require(!code,"touch display buffer sizes");
 std::vector<DISPLAYCONFIG_PATH_INFO> paths(pathsCount);std::vector<DISPLAYCONFIG_MODE_INFO> modes(modesCount);
 code=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&pathsCount,paths.data(),&modesCount,modes.data(),nullptr);P::Require(!code,"touch display query");
 Target result{};unsigned found=0;
 for(UINT32 i=0;i<pathsCount;++i){const auto& path=paths[i];DISPLAYCONFIG_TARGET_DEVICE_NAME target{};target.header={DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,sizeof(target),path.targetInfo.adapterId,path.targetInfo.id};
  P::Require(!DisplayConfigGetDeviceInfo(&target.header),"touch target identity");std::wstring identity=target.monitorDevicePath;std::transform(identity.begin(),identity.end(),identity.begin(),towupper);
  if(identity.find(L"SWT0001")==std::wstring::npos)continue;
  DISPLAYCONFIG_SOURCE_DEVICE_NAME source{};source.header={DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,sizeof(source),path.sourceInfo.adapterId,path.sourceInfo.id};P::Require(!DisplayConfigGetDeviceInfo(&source.header),"touch source identity");
  DEVMODEW mode{};mode.dmSize=sizeof(mode);P::Require(EnumDisplaySettingsExW(source.viewGdiDeviceName,ENUM_CURRENT_SETTINGS,&mode,0)!=FALSE,"touch display mode");
  P::Require(path.targetInfo.outputTechnology==DISPLAYCONFIG_OUTPUT_TECHNOLOGY_INDIRECT_WIRED,"touch indirect target");
  result.left=mode.dmPosition.x;result.top=mode.dmPosition.y;result.width=mode.dmPelsWidth;result.height=mode.dmPelsHeight;result.adapter=path.sourceInfo.adapterId;result.source=path.sourceInfo.id;result.target=path.targetInfo.id;++found;
 }
 P::Require(found==1&&result.width==2400&&result.height==1080,"unique 2400x1080 SweetDisplay touch target");
 uint64_t h=1469598103934665603ULL;h=Mix(h,uint32_t(result.adapter.HighPart));h=Mix(h,result.adapter.LowPart);h=Mix(h,result.source);h=Mix(h,result.target);h=Mix(h,uint32_t(result.left));h=Mix(h,uint32_t(result.top));h=Mix(h,result.width);h=Mix(h,result.height);result.token=h?h:1;return result;
}

class ApiFailure final:public P::Violation {
public:
 DWORD error;
 explicit ApiFailure(DWORD value):P::Violation("InjectTouchInput error "+std::to_string(value)),error(value){}
};
class Session final:public T::Endpoint {
 Mode mode;FILE* evidence=nullptr;FILE* api=nullptr;Target target{};uint64_t session=0;std::unique_ptr<T::ContactState> state;
 std::array<T::Event,2> desired{},delivered{};std::array<bool,2> deliveredValid{};uint32_t injectedMask=0,pendingMask=0;uint64_t pendingTimestamp=0,rows=0,injected=0,released=0;bool pending=false,initialized=false,releaseFailed=false,endAttempted=false;DWORD releaseError=ERROR_SUCCESS;ULONGLONG nextKeepAlive=0;std::thread::id owner{};
 static constexpr ULONGLONG KeepAliveMs=50;
 static const char* ActionName(T::Action a){switch(a){case T::Action::Down:return "DOWN";case T::Action::Move:return "MOVE";case T::Action::Up:return "UP";case T::Action::Cancel:return "CANCEL";}return "INVALID";}
 void ClaimOwner(){const auto current=std::this_thread::get_id();if(owner==std::thread::id{})owner=current;P::Require(owner==current,"touch injection owner thread");}
 void ApiRecord(const char* stage,const std::array<POINTER_TOUCH_INFO,2>& contacts,UINT32 count,BOOL ok,DWORD error,unsigned attempt){if(!api)return;for(UINT32 i=0;i<count;++i){const auto& p=contacts[i].pointerInfo;fprintf(api,"%s,%llu,%u,%u,%lu,%ld,%ld,%d,%lu,%u\n",stage,session,count,p.pointerId,p.pointerFlags,p.ptPixelLocation.x,p.ptPixelLocation.y,ok?1:0,error,attempt);}fflush(api);}
 POINT Map(const T::Event& event)const {return {target.left+LONG((uint64_t(event.x)*(target.width-1)+T::CoordinateMaximum/2)/T::CoordinateMaximum),target.top+LONG((uint64_t(event.y)*(target.height-1)+T::CoordinateMaximum/2)/T::CoordinateMaximum)};}
 POINTER_TOUCH_INFO Info(unsigned slot,const T::Event& event,POINTER_FLAGS transition)const {auto point=Map(event);POINTER_TOUCH_INFO info{};info.pointerInfo.pointerType=PT_TOUCH;info.pointerInfo.pointerId=slot;info.pointerInfo.pointerFlags=transition;info.pointerInfo.ptPixelLocation=point;info.touchFlags=TOUCH_FLAG_NONE;info.touchMask=TOUCH_MASK_CONTACTAREA;info.rcContact={point.x-4,point.y-4,point.x+4,point.y+4};return info;}
 void EnsureInitialized(){if(mode!=Mode::Inject||initialized)return;SetLastError(ERROR_SUCCESS);const BOOL ok=InitializeTouchInjection(T::MaximumContacts,TOUCH_FEEDBACK_NONE);const DWORD error=ok?ERROR_SUCCESS:GetLastError();if(api)fprintf(api,"INITIALIZE,%llu,0,0,0,0,0,%d,%lu,0\n",session,ok?1:0,error);if(api)fflush(api);P::Require(ok,"InitializeTouchInjection");initialized=true;}
 void Submit(const char* stage,std::array<POINTER_TOUCH_INFO,2>& contacts,UINT32 count){P::Require(count&&count<=T::MaximumContacts,"InjectTouchInput count");EnsureInitialized();DWORD error=ERROR_SUCCESS;for(unsigned attempt=0;attempt<3;++attempt){SetLastError(ERROR_SUCCESS);if(InjectTouchInput(count,contacts.data())){ApiRecord(stage,contacts,count,TRUE,ERROR_SUCCESS,attempt);++injected;return;}error=GetLastError();ApiRecord(stage,contacts,count,FALSE,error,attempt);if(error!=ERROR_NOT_READY||attempt==2)throw ApiFailure(error);Sleep(2);}}
 void Update(uint32_t mask,const char* stage){if(!mask||mode!=Mode::Inject)return;std::array<POINTER_TOUCH_INFO,2> contacts{};UINT32 count=0;for(unsigned slot=0;slot<T::MaximumContacts;++slot)if(mask&(1u<<slot))contacts[count++]=Info(slot,desired[slot],POINTER_FLAG_INRANGE|POINTER_FLAG_INCONTACT|POINTER_FLAG_UPDATE);Submit(stage,contacts,count);for(unsigned slot=0;slot<T::MaximumContacts;++slot)if(mask&(1u<<slot)){delivered[slot]=desired[slot];deliveredValid[slot]=true;}nextKeepAlive=GetTickCount64()+KeepAliveMs;}
 void SendDown(const T::Event& event){std::array<POINTER_TOUCH_INFO,2> contacts{};UINT32 count=0;const uint32_t include=injectedMask|(1u<<event.contact);for(unsigned slot=0;slot<T::MaximumContacts;++slot)if(include&(1u<<slot)){const bool down=slot==event.contact;contacts[count++]=Info(slot,desired[slot],down?POINTER_FLAG_INRANGE|POINTER_FLAG_INCONTACT|POINTER_FLAG_DOWN:POINTER_FLAG_INRANGE|POINTER_FLAG_INCONTACT|POINTER_FLAG_UPDATE);}Submit("DOWN",contacts,count);for(unsigned slot=0;slot<T::MaximumContacts;++slot)if(injectedMask&(1u<<slot)){delivered[slot]=desired[slot];deliveredValid[slot]=true;}injectedMask|=1u<<event.contact;deliveredValid[event.contact]=false;nextKeepAlive=GetTickCount64()+KeepAliveMs;}
 void SendTerminal(const T::Event& event,uint32_t before){P::Require(before==injectedMask&&before&(1u<<event.contact),"touch injected/contact state");Update(before,"TERMINAL_UPDATE");std::array<POINTER_TOUCH_INFO,2> contacts{};UINT32 count=0;for(unsigned slot=0;slot<T::MaximumContacts;++slot)if(before&(1u<<slot)){const bool terminal=slot==event.contact;const auto flags=terminal?(POINTER_FLAG_UP|(event.action==T::Action::Cancel?POINTER_FLAG_CANCELED:0)):POINTER_FLAG_INRANGE|POINTER_FLAG_INCONTACT|POINTER_FLAG_UPDATE;contacts[count++]=Info(slot,terminal?delivered[slot]:desired[slot],flags);}Submit("UP",contacts,count);injectedMask&=~(1u<<event.contact);deliveredValid[event.contact]=false;}
 bool FlushPending(){if(!pending)return false;Update(pendingMask,"MOVE");pending=false;pendingMask=0;pendingTimestamp=0;return true;}
 bool Release(bool record)noexcept{if(!injectedMask)return true;try{Update(injectedMask,"RELEASE_UPDATE");std::array<POINTER_TOUCH_INFO,2> contacts{};UINT32 count=0;for(unsigned slot=0;slot<T::MaximumContacts;++slot)if(injectedMask&(1u<<slot))contacts[count++]=Info(slot,delivered[slot],POINTER_FLAG_UP);Submit("RELEASE_UP",contacts,count);injectedMask=0;deliveredValid.fill(false);pending=false;pendingMask=0;pendingTimestamp=0;if(record&&evidence){fprintf(evidence,"RELEASE_ALL,%llu,0,0,0,0,0,0,0,0,%llu\n",session,++released);fflush(evidence);}return true;}catch(const ApiFailure& failure){releaseFailed=true;releaseError=failure.error;if(evidence){fprintf(evidence,"RELEASE_UNCERTAIN,%llu,0,0,0,0,0,0,0,%lu,%llu\n",session,releaseError,++released);fflush(evidence);}return false;}catch(...){releaseFailed=true;releaseError=ERROR_GEN_FAILURE;if(evidence){fprintf(evidence,"RELEASE_UNCERTAIN,%llu,0,0,0,0,0,0,0,%lu,%llu\n",session,releaseError,++released);fflush(evidence);}return false;}}
 void Log(const T::Event& event){auto point=Map(event);const LONG localX=point.x-target.left,localY=point.y-target.top;P::Require(++rows<=10000,"bounded touch evidence exhausted");fprintf(evidence,"%s,%llu,%u,%u,%u,%ld,%ld,%ld,%ld,%llu\n",ActionName(event.action),session,event.contact,event.x,event.y,localX,localY,point.x,point.y,event.deviceTimestampNs);fflush(evidence);}
public:
 Session(Mode value,const std::wstring& directory):mode(value){const auto path=directory+L"/touch-events.csv",apiPath=directory+L"/touch-api.csv";if(std::filesystem::exists(path)||std::filesystem::exists(apiPath))throw std::runtime_error("refusing existing touch evidence");evidence=_wfsopen(path.c_str(),L"w",_SH_DENYWR);api=_wfsopen(apiPath.c_str(),L"w",_SH_DENYWR);if(!evidence||!api)throw std::runtime_error("unwritable touch evidence error "+std::to_string(errno));fprintf(evidence,"event,session,contact,normalized_x,normalized_y,local_x,local_y,desktop_x,desktop_y,detail\n");fprintf(api,"stage,session,count,pointer_id,pointer_flags,x,y,success,error,attempt\n");fflush(evidence);fflush(api);}
 ~Session(){End();if(api)fclose(api);if(evidence)fclose(evidence);}
 T::Configuration CurrentConfiguration()override{target=Discover();return {T::CoordinateMaximum,target.width,target.height,T::MaximumContacts,T::RequiredConfigFlags,target.token};}
 void Begin(uint64_t id,const T::Configuration& configuration)override{End();P::Require(!state&&!injectedMask&&!releaseFailed,"previous touch release uncertain");ClaimOwner();target=Discover();P::Require(configuration.targetToken==target.token&&configuration.width==target.width&&configuration.height==target.height,"touch target changed before session");session=id;state=std::make_unique<T::ContactState>(configuration);desired={};delivered={};deliveredValid.fill(false);endAttempted=false;fprintf(evidence,"SESSION_READY,%llu,0,0,0,0,0,%ld,%ld,%llu\n",session,target.left,target.top,target.token);fflush(evidence);}
 void Accept(uint64_t id,const T::Event& event)override{ClaimOwner();P::Require(state&&!releaseFailed&&id==session,"stale/blocked touch session");const auto current=Discover();if(current.token!=target.token){End();throw P::Violation("touch display topology changed");}const bool terminal=event.action==T::Action::Up||event.action==T::Action::Cancel;const uint32_t before=injectedMask;state->Apply(event);Log(event);if(event.action==T::Action::Down||event.action==T::Action::Move)desired[event.contact]=event;if(event.action==T::Action::Down){P::Require(!(before&(1u<<event.contact)),"duplicate injected contact");if(mode==Mode::Inject)SendDown(event);return;}if(event.action==T::Action::Move){P::Require(before&(1u<<event.contact),"MOVE without injected contact");pending=true;pendingMask=before;pendingTimestamp=event.deviceTimestampNs;return;}P::Require(terminal,"invalid touch action");if(pending)FlushPending();if(mode==Mode::Inject)SendTerminal(event,before);}
 void Tick()override{ClaimOwner();if(!state||releaseFailed||!injectedMask)return;if(pending)FlushPending();if(GetTickCount64()>=nextKeepAlive)Update(injectedMask,"KEEPALIVE");}
 void End()noexcept override{if(!state){return;}if(owner!=std::thread::id{}&&owner!=std::this_thread::get_id()){releaseFailed=true;releaseError=ERROR_INVALID_THREAD_ID;if(evidence){fprintf(evidence,"RELEASE_UNCERTAIN,%llu,0,0,0,0,0,0,0,%lu,%llu\n",session,releaseError,++released);fflush(evidence);}return;}if(endAttempted)return;endAttempted=true;try{if(pending)FlushPending();}catch(const ApiFailure& failure){releaseFailed=true;releaseError=failure.error;}catch(...){releaseFailed=true;releaseError=ERROR_GEN_FAILURE;}if(!releaseFailed&&!Release(true))return;if(!releaseFailed){state->ReleaseAll();state.reset();session=0;}}
 uint64_t Injected()const{return injected;}uint32_t ActiveMask()const{return injectedMask;}bool ReleaseClean()const{return !releaseFailed&&!injectedMask;}DWORD ReleaseError()const{return releaseError;}
};
}
