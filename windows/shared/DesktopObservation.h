// Read-only documented user-mode observations. No application hooks or desktop switch.
#pragma once
#include <windows.h>
#include <dwmapi.h>
#include <cstdio>
#include <string>
#include <vector>
#include <algorithm>
#pragma comment(lib,"dwmapi.lib")
namespace SweetDisplay::Observation {
inline uint64_t Now(){LARGE_INTEGER q{};QueryPerformanceCounter(&q);return q.QuadPart;}
struct Snapshot {
 uint64_t foreground=0,pattern=0;DWORD foregroundPid=0,patternPid=0,desktopError=0,cloaked=0;HRESULT cloakResult=0,compositionResult=0;
 BOOL visible=FALSE,iconic=FALSE,composition=FALSE;RECT rectangle{};wchar_t desktop[80]{};
};
inline Snapshot Read(HWND pattern){
 Snapshot s{};auto fg=GetForegroundWindow();s.foreground=uint64_t(fg);s.pattern=uint64_t(pattern);
 GetWindowThreadProcessId(fg,&s.foregroundPid);GetWindowThreadProcessId(pattern,&s.patternPid);
 s.visible=IsWindowVisible(pattern);s.iconic=IsIconic(pattern);GetWindowRect(pattern,&s.rectangle);
 s.cloakResult=DwmGetWindowAttribute(pattern,DWMWA_CLOAKED,&s.cloaked,sizeof(s.cloaked));s.compositionResult=DwmIsCompositionEnabled(&s.composition);
 HDESK desktop=OpenInputDesktop(0,FALSE,DESKTOP_READOBJECTS);
 if(desktop){DWORD bytes=0;if(!GetUserObjectInformationW(desktop,UOI_NAME,s.desktop,sizeof(s.desktop),&bytes))s.desktopError=GetLastError();CloseDesktop(desktop);}else s.desktopError=GetLastError();return s;
}
inline void Write(FILE* file,HWND pattern,const char* event,uint64_t value=0){
 if(!file)return;const auto s=Read(pattern);
 // Window titles are deliberately not collected. Desktop name normally Default/Winlogon.
 fprintf(file,"%llu,%s,%llu,%llu,%lu,%llu,%lu,%d,%d,%lu,0x%08lX,%d,0x%08lX,%lu,%ls,%ld,%ld,%ld,%ld\n",Now(),event,value,s.foreground,s.foregroundPid,s.pattern,s.patternPid,s.visible,s.iconic,s.cloaked,ULONG(s.cloakResult),s.composition,ULONG(s.compositionResult),s.desktopError,s.desktop,s.rectangle.left,s.rectangle.top,s.rectangle.right,s.rectangle.bottom);fflush(file);
}
inline void Header(FILE* f){fprintf(f,"qpc,event,value,foreground_hwnd,foreground_pid,pattern_hwnd,pattern_pid,visible,minimized,cloaked,cloaked_hresult,composition,composition_hresult,input_desktop_error,input_desktop,rect_left,rect_top,rect_right,rect_bottom\n");}
inline void Display(FILE* f){
 UINT32 n=0,m=0;LONG code=GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&n,&m);
 if(code){fprintf(f,"query_error=%ld\n",code);return;}
 std::vector<DISPLAYCONFIG_PATH_INFO> paths(n);std::vector<DISPLAYCONFIG_MODE_INFO> modes(m);
 code=QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&n,paths.data(),&m,modes.data(),nullptr);if(code){fprintf(f,"query_error=%ld\n",code);return;}
 for(UINT32 i=0;i<n;++i){const auto& p=paths[i];DISPLAYCONFIG_TARGET_DEVICE_NAME t{};t.header={DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,sizeof(t),p.targetInfo.adapterId,p.targetInfo.id};
  if(DisplayConfigGetDeviceInfo(&t.header))continue;std::wstring identity=t.monitorDevicePath;std::transform(identity.begin(),identity.end(),identity.begin(),towupper);
  if(identity.find(L"SWT0001")==std::wstring::npos)continue;
  DISPLAYCONFIG_SOURCE_DEVICE_NAME s{};s.header={DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,sizeof(s),p.sourceInfo.adapterId,p.sourceInfo.id};code=DisplayConfigGetDeviceInfo(&s.header);
  DEVMODEW mode{};mode.dmSize=sizeof(mode);BOOL available=!code&&EnumDisplaySettingsExW(s.viewGdiDeviceName,ENUM_CURRENT_SETTINGS,&mode,0);
  fprintf(f,"qpc=%llu source_name=%ls monitor=%ls friendly=%ls mode_available=%d width=%lu height=%lu left=%ld top=%ld refresh=%u/%u\n",Now(),s.viewGdiDeviceName,t.monitorDevicePath,t.monitorFriendlyDeviceName,available,mode.dmPelsWidth,mode.dmPelsHeight,mode.dmPosition.x,mode.dmPosition.y,p.targetInfo.refreshRate.Numerator,p.targetInfo.refreshRate.Denominator);
 }fflush(f);
}
}
