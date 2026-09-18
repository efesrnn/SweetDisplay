// Diagnostic events and benign controls for our own test windows only.
#pragma once
#include "../shared/PatternOracle.h"
#include "../shared/DesktopObservation.h"
#include <wtsapi32.h>
#include <dbt.h>
#pragma comment(lib,"wtsapi32.lib")
namespace SweetDisplay {
class PatternObservation {
 FILE* events=nullptr;FILE* topology=nullptr;HWND window=nullptr,control=nullptr;HPOWERNOTIFY power=nullptr;HDEVNOTIFY devices=nullptr;bool wts=false,triggered=false;
 uint32_t nonce=0,oldCounter=0;uint64_t connectedQpc=0,lastTopology=0;std::wstring mode;HRESULT previousOcclusion=E_UNEXPECTED;
 static LRESULT CALLBACK ControlProc(HWND w,UINT m,WPARAM a,LPARAM b){
  auto* self=reinterpret_cast<PatternObservation*>(GetWindowLongPtrW(w,GWLP_USERDATA));
  if(m==WM_NCCREATE){self=static_cast<PatternObservation*>(reinterpret_cast<CREATESTRUCTW*>(b)->lpCreateParams);SetWindowLongPtrW(w,GWLP_USERDATA,LONG_PTR(self));}
  if(m==WM_PAINT&&self){PAINTSTRUCT p{};HDC dc=BeginPaint(w,&p);RECT rect{};GetClientRect(w,&rect);HBRUSH grey=CreateSolidBrush(RGB(48,48,48));FillRect(dc,&rect,grey);DeleteObject(grey);
   if(self->mode==L"snapshot")for(int row=0;row<2;++row)for(int bit=0;bit<32;++bit){RECT cell{8+bit*20,8+row*48,24+bit*20,40+row*48};uint32_t value=row?self->oldCounter:self->nonce;FillRect(dc,&cell,HBRUSH(GetStockObject(value&(1u<<bit)?WHITE_BRUSH:BLACK_BRUSH)));}
   EndPaint(w,&p);return 0;
  }
  return DefWindowProcW(w,m,a,b);
 }
 void Trigger(){
  triggered=true;Observation::Write(events,window,"control_begin",PatternOracle::Load(&ledger.data->hostCounter));
  if(mode==L"minimize"){ShowWindow(window,SW_MINIMIZE);Observation::Write(events,window,"controlled_minimize");return;}
  RECT r{};GetWindowRect(window,&r);oldCounter=PatternOracle::Load(&ledger.data->hostCounter)-26;
  WNDCLASSW c{};c.hInstance=GetModuleHandleW(nullptr);c.lpfnWndProc=ControlProc;c.lpszClassName=L"SweetDisplayControlledOverlay";
  if(!RegisterClassW(&c)&&GetLastError()!=ERROR_CLASS_ALREADY_EXISTS)throw std::runtime_error("Control window class failed");
  const bool foreground=mode==L"foreground";
  control=CreateWindowExW(WS_EX_TOOLWINDOW,c.lpszClassName,L"SweetDisplay controlled desktop transition",WS_POPUP,r.left+(foreground?800:24),r.top+(foreground?200:24),foreground?300:656,foreground?200:96,nullptr,nullptr,c.hInstance,this);
  if(!control)throw std::runtime_error("Control window creation failed");
  if(!SetWindowPos(control,HWND_TOPMOST,0,0,0,0,SWP_NOMOVE|SWP_NOSIZE|SWP_SHOWWINDOW|SWP_NOACTIVATE))throw std::runtime_error("Control window placement failed");
  InvalidateRect(control,nullptr,FALSE);UpdateWindow(control);
  if(foreground)Observation::Write(events,window,"controlled_foreground_result",SetForegroundWindow(control));
  Observation::Write(events,window,mode==L"snapshot"?"controlled_snapshot_counter":"controlled_grey_overlay",mode==L"snapshot"?oldCounter:uint64_t(control));
 }
public:
 PatternOracle::Mapping ledger;bool enabled=false;
 ~PatternObservation(){if(control&&IsWindow(control))DestroyWindow(control);StopNotifications();if(events)fclose(events);if(topology)fclose(topology);}
 void Start(HWND w,const std::wstring& dir,uint32_t n,const std::wstring& selected){
  if(selected.empty())return;
  if(selected!=L"observe"&&selected!=L"cover"&&selected!=L"snapshot"&&selected!=L"minimize"&&selected!=L"foreground")throw std::runtime_error("Unknown controlled diagnostic case");
  enabled=true;window=w;nonce=n;mode=selected;ledger.Open(n,true,w);
  if(_wfopen_s(&events,(dir+L"/desktop-events.csv").c_str(),L"w")||!events)throw std::runtime_error("Cannot open desktop telemetry");
  if(_wfopen_s(&topology,(dir+L"/display-transitions.txt").c_str(),L"w")||!topology)throw std::runtime_error("Cannot open display telemetry");
  Observation::Header(events);Observation::Write(events,window,"pattern_created",GetCurrentProcessId());Observation::Display(topology);
  wts=WTSRegisterSessionNotification(window,NOTIFY_FOR_THIS_SESSION)!=FALSE;Observation::Write(events,window,"session_registration",wts?0:GetLastError());
  power=RegisterPowerSettingNotification(window,&GUID_CONSOLE_DISPLAY_STATE,DEVICE_NOTIFY_WINDOW_HANDLE);Observation::Write(events,window,"display_power_registration",power?0:GetLastError());
  DEV_BROADCAST_DEVICEINTERFACE_W filter{};filter.dbcc_size=sizeof(filter);filter.dbcc_devicetype=DBT_DEVTYP_DEVICEINTERFACE;
  devices=RegisterDeviceNotificationW(window,&filter,DEVICE_NOTIFY_WINDOW_HANDLE|DEVICE_NOTIFY_ALL_INTERFACE_CLASSES);Observation::Write(events,window,"device_registration",devices?0:GetLastError());
  if(!SetTimer(window,301,100,nullptr))throw std::runtime_error("Diagnostic observation timer failed");
 }
 void StopNotifications(){if(window){KillTimer(window,301);if(wts){WTSUnRegisterSessionNotification(window);wts=false;}}if(power){UnregisterPowerSettingNotification(power);power=nullptr;}if(devices){UnregisterDeviceNotification(devices);devices=nullptr;}}
 void Message(UINT m,WPARAM a,LPARAM b){
  if(!enabled||!events)return;
  if(m==WM_TIMER&&a==301){
   Observation::Write(events,window,"poll");const auto now=Observation::Now();
   if(now-lastTopology>=ledger.data->frequency*2){Observation::Display(topology);lastTopology=now;}
   if(mode!=L"observe"&&!triggered&&PatternOracle::Load(&ledger.data->hostCounter)>26){if(!connectedQpc)connectedQpc=now;if(now-connectedQpc>=ledger.data->frequency*8)Trigger();}
  }
  if(m==WM_WTSSESSION_CHANGE)Observation::Write(events,window,"session_change",a);
  if(m==WM_DISPLAYCHANGE){Observation::Write(events,window,"display_change",uint64_t(b));Observation::Display(topology);}
  if(m==WM_DEVICECHANGE)Observation::Write(events,window,"device_change",a);
  if(m==WM_SIZE)Observation::Write(events,window,"size_or_minimize",a);
  if(m==WM_SHOWWINDOW)Observation::Write(events,window,"show_window",a);
  if(m==WM_WINDOWPOSCHANGED)Observation::Write(events,window,"window_position");
  if(m==WM_POWERBROADCAST){Observation::Write(events,window,"power_notification",a);if(a==PBT_POWERSETTINGCHANGE&&b){auto* p=reinterpret_cast<POWERBROADCAST_SETTING*>(b);if(p->DataLength==sizeof(DWORD)&&IsEqualGUID(p->PowerSetting,GUID_CONSOLE_DISPLAY_STATE)){DWORD state;memcpy(&state,p->Data,sizeof(state));Observation::Write(events,window,"console_display_state",state);}}}
  if(m==WM_DESTROY){Observation::Write(events,window,"pattern_destroyed");StopNotifications();}
 }
 void Occlusion(HRESULT hr){if(enabled&&hr!=previousOcclusion){Observation::Write(events,window,"dxgi_present_test_hresult",ULONG(hr));previousOcclusion=hr;}}
};
}
