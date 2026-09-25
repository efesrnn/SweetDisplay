@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
set "simout=out\device-simulator"
set "hostout=out\transport-host"
set "fixtureout=out\transport-fixture"
if /i "%~3"=="review" (
 set "simout=out\device-simulator-review"
 set "hostout=out\transport-host-review"
 set "fixtureout=out\transport-fixture-review"
)
if /i "%~3"=="content-v2" (
 set "simout=out\device-simulator-content-v2"
 set "hostout=out\transport-host-content-v2"
 set "fixtureout=out\transport-fixture-content-v2"
)
if /i "%~3"=="perf1" (
 set "simout=out\device-simulator-perf1"
 set "hostout=out\transport-host-perf1"
 set "fixtureout=out\transport-fixture-perf1"
)
if /i "%~3"=="perf2" (
 set "simout=out\device-simulator-perf2"
 set "hostout=out\transport-host-perf2"
 set "fixtureout=out\transport-fixture-perf2"
)
if /i "%~3"=="perf3" (
 set "simout=out\device-simulator-perf3"
 set "hostout=out\transport-host-perf3"
 set "fixtureout=out\transport-fixture-perf3"
)
if not exist "%simout%" mkdir "%simout%"
set "sdk=%~1\Program Files\Windows Kits\10"
set "includes=/I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" /I"%sdk%\Include\10.0.26100.0\winrt""
set "libraries=/LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64""
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 %includes% windows\tools\SweetDisplayDeviceSimulator.cpp /Fo"%simout%\SweetDisplayDeviceSimulator.obj" /Fe"%simout%\SweetDisplayDeviceSimulator.exe" /link %libraries% ws2_32.lib bcrypt.lib
if errorlevel 1 exit /b 1
if not exist "%fixtureout%" mkdir "%fixtureout%"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 %includes% windows\protocol\tests\TransportFixture.cpp /Fo"%fixtureout%\TransportFixture.obj" /Fe"%fixtureout%\TransportFixture.exe" /link %libraries% ws2_32.lib bcrypt.lib
if errorlevel 1 exit /b 1
if "%~2"=="endpoint-only" exit /b 0
if not exist "%hostout%" mkdir "%hostout%"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 /DSWEETDISPLAY_TRANSPORT %includes% windows\host\SweetDisplayHost.cpp /Fo"%hostout%\SweetDisplayHost.obj" /Fe"%hostout%\SweetDisplayHost.exe" /link %libraries% mfplat.lib mf.lib mfuuid.lib d3d11.lib dxgi.lib ole32.lib oleaut32.lib user32.lib gdi32.lib advapi32.lib setupapi.lib winmm.lib ws2_32.lib bcrypt.lib
set "result=%errorlevel%"
popd
exit /b %result%
