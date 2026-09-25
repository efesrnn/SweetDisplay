@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
set "output=out\device-simulator-visual-async-v3"
if not exist "%output%" mkdir "%output%"
set "sdk=%~1\Program Files\Windows Kits\10"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 /I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" /I"%sdk%\Include\10.0.26100.0\winrt" windows\simulator\SweetDisplayDeviceSimulator.cpp /Fo"%output%\SweetDisplayDeviceSimulator.obj" /Fe"%output%\SweetDisplayDeviceSimulator.exe" /link /LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64" ws2_32.lib bcrypt.lib mfplat.lib mf.lib mfuuid.lib d3d11.lib dxgi.lib ole32.lib oleaut32.lib user32.lib gdi32.lib psapi.lib wmcodecdspuuid.lib
set "result=%errorlevel%"
popd
exit /b %result%

