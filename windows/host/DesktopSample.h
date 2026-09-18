// Independent screen-DC corroboration of two diagnostic rows; no image files.
#pragma once
#include "../shared/DesktopObservation.h"
#include <array>
namespace SweetDisplay::DesktopSample {
using Pixels=std::array<BYTE,640*2*4>;
inline uint64_t RgbHash(const Pixels& p){uint64_t h=1469598103934665603ull;for(size_t i=0;i<p.size();++i)if(i%4!=3){h^=p[i];h*=1099511628211ull;}return h;}
inline bool Equal(const Pixels& a,const Pixels& b){for(size_t i=0;i<a.size();++i)if(i%4!=3&&a[i]!=b[i])return false;return true;}
inline bool BinaryCells(const Pixels& p,uint32_t nonce,uint32_t counter){
 for(unsigned row=0;row<2;++row)for(unsigned bit=0;bit<32;++bit)for(unsigned x=0;x<16;++x){
  const BYTE expected=((row?counter:nonce)&(1u<<bit))?255:0;
  for(unsigned c=0;c<3;++c)if(p[(row*640+bit*20+x)*4+c]!=expected)return false;
 }return true;
}
struct Result {uint64_t begin=0,end=0,hash1=0,hash2=0;DWORD error=0;LONG x=0,y=0;bool matches=false;};
inline bool Target(POINT& point){
 UINT32 n=0,m=0;if(GetDisplayConfigBufferSizes(QDC_ONLY_ACTIVE_PATHS,&n,&m))return false;
 std::vector<DISPLAYCONFIG_PATH_INFO> paths(n);std::vector<DISPLAYCONFIG_MODE_INFO> modes(m);
 if(QueryDisplayConfig(QDC_ONLY_ACTIVE_PATHS,&n,paths.data(),&m,modes.data(),nullptr))return false;
 unsigned found=0;
 for(UINT32 i=0;i<n;++i){const auto& path=paths[i];DISPLAYCONFIG_TARGET_DEVICE_NAME t{};
  t.header={DISPLAYCONFIG_DEVICE_INFO_GET_TARGET_NAME,sizeof(t),path.targetInfo.adapterId,path.targetInfo.id};
  if(DisplayConfigGetDeviceInfo(&t.header))return false;std::wstring id=t.monitorDevicePath;std::transform(id.begin(),id.end(),id.begin(),towupper);
  if(id.find(L"SWT0001")==std::wstring::npos)continue;
  DISPLAYCONFIG_SOURCE_DEVICE_NAME s{};s.header={DISPLAYCONFIG_DEVICE_INFO_GET_SOURCE_NAME,sizeof(s),path.sourceInfo.adapterId,path.sourceInfo.id};
  DEVMODEW mode{};mode.dmSize=sizeof(mode);
  if(DisplayConfigGetDeviceInfo(&s.header)||!EnumDisplaySettingsExW(s.viewGdiDeviceName,ENUM_CURRENT_SETTINGS,&mode,0)||mode.dmPelsWidth!=2400||mode.dmPelsHeight!=1080)return false;
  point.x=mode.dmPosition.x;point.y=mode.dmPosition.y;++found;
 }return found==1;
}
inline Result Compare(const Pixels& actual){
 Result r{};r.begin=Observation::Now();POINT point{};
 // Coordinates are physical screen pixels, independent of the Host's DPI context.
 auto oldDpi=SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);
 HDC screen=nullptr,memory=nullptr;HBITMAP bitmap=nullptr;HGDIOBJ previous=nullptr;void* bits=nullptr;
 if(!oldDpi||!Target(point)){r.error=ERROR_NOT_FOUND;}
 else {
  r.x=point.x+32;r.y=point.y+48;screen=GetDC(nullptr);
  if(screen)memory=CreateCompatibleDC(screen);
  BITMAPINFO info{};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=640;info.bmiHeader.biHeight=-2;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
  if(memory)bitmap=CreateDIBSection(screen,&info,DIB_RGB_COLORS,&bits,nullptr,0);
  if(bitmap)previous=SelectObject(memory,bitmap);
  if(!screen||!memory||!bitmap||!previous||previous==HGDI_ERROR)r.error=GetLastError()?GetLastError():ERROR_INVALID_HANDLE;
  else {
   Pixels captures[2]{};bool ok=true;
   for(int i=0;i<2&&ok;++i){
    ok=BitBlt(memory,0,0,640,1,screen,point.x+32,point.y+48,SRCCOPY|CAPTUREBLT|NOMIRRORBITMAP)&&BitBlt(memory,0,1,640,1,screen,point.x+32,point.y+96,SRCCOPY|CAPTUREBLT|NOMIRRORBITMAP)&&GdiFlush();
    if(ok)memcpy(captures[i].data(),bits,captures[i].size());else r.error=GetLastError()?GetLastError():ERROR_READ_FAULT;
   }
   if(ok){r.hash1=RgbHash(captures[0]);r.hash2=RgbHash(captures[1]);r.matches=Equal(actual,captures[0])&&Equal(actual,captures[1]);}
  }
 }
 if(previous&&previous!=HGDI_ERROR)SelectObject(memory,previous);
 if(bitmap)DeleteObject(bitmap);if(memory)DeleteDC(memory);if(screen)ReleaseDC(nullptr,screen);if(oldDpi)SetThreadDpiAwarenessContext(oldDpi);
 r.end=Observation::Now();return r;
}
}
