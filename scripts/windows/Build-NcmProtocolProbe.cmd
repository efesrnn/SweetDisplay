@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
set "out=out\ncm-protocol-probe"
if not exist "%out%" mkdir "%out%"
set "sdk=%~1\Program Files\Windows Kits\10"
set "includes=/I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" /I"%sdk%\Include\10.0.26100.0\winrt""
set "libraries=/LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64""
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 %includes% windows\tools\NcmProtocolProbe.cpp /Fo"%out%\NcmProtocolProbe.obj" /Fe"%out%\NcmProtocolProbe.exe" /link %libraries% ws2_32.lib bcrypt.lib
set "result=%errorlevel%"
popd
exit /b %result%
