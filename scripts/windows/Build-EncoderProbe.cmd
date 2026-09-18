@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
if not exist out\phase3b\probe mkdir out\phase3b\probe
set "sdk=%~1\Program Files\Windows Kits\10"
cl /nologo /std:c++17 /EHsc /W4 /utf-8 /O2 /I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" /I"%sdk%\Include\10.0.26100.0\winrt" windows\tools\EncoderDeviceProbe.cpp /Foout\phase3b\probe\EncoderDeviceProbe.obj /Feout\phase3b\probe\EncoderDeviceProbe.exe /link /LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64" mfplat.lib mf.lib mfuuid.lib d3d11.lib dxgi.lib ole32.lib
set "result=%errorlevel%"
popd
exit /b %result%
