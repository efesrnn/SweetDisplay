@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
if not exist out\encode-host-control mkdir out\encode-host-control
set "sdk=%~1\Program Files\Windows Kits\10"
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /O2 /I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" /I"%sdk%\Include\10.0.26100.0\winrt" windows\host\SweetDisplayHost.cpp /Foout\encode-host-control\SweetDisplayHost.obj /Feout\encode-host-control\SweetDisplayHost.exe /link /LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64" mfplat.lib mf.lib mfuuid.lib d3d11.lib dxgi.lib ole32.lib oleaut32.lib user32.lib gdi32.lib advapi32.lib setupapi.lib winmm.lib
set "result=%errorlevel%"
popd
exit /b %result%
