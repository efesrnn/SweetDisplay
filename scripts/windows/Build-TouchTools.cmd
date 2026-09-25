@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
set "sdk=%~1\Program Files\Windows Kits\10"
set "includes=/I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" /I"%sdk%\Include\10.0.26100.0\winrt""
set "libraries=/LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64""
if not exist "out\gpu-pattern" mkdir "out\gpu-pattern"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 %includes% windows\tools\GpuFramePattern.cpp /Fo"out\gpu-pattern\GpuFramePattern.obj" /Fe"out\gpu-pattern\SweetDisplayGpuPattern.exe" /link %libraries% user32.lib gdi32.lib d3d11.lib dxgi.lib dwmapi.lib
if errorlevel 1 goto failed
if not exist "out\touch-release-harness" mkdir "out\touch-release-harness"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 %includes% windows\tools\TouchReleaseHarness.cpp /Fo"out\touch-release-harness\TouchReleaseHarness.obj" /Fe"out\touch-release-harness\TouchReleaseHarness.exe" /link %libraries% user32.lib
if errorlevel 1 goto failed
if not exist "out\transport-host" mkdir "out\transport-host"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 /DSWEETDISPLAY_TRANSPORT %includes% windows\host\SweetDisplayHost.cpp /Fo"out\transport-host\SweetDisplayHost.obj" /Fe"out\transport-host\SweetDisplayHost.exe" /link %libraries% mfplat.lib mf.lib mfuuid.lib d3d11.lib dxgi.lib ole32.lib oleaut32.lib user32.lib gdi32.lib advapi32.lib setupapi.lib winmm.lib ws2_32.lib bcrypt.lib
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
