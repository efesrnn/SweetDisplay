@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
set "testout=out\encoder-worker-tests"
if not exist "%testout%" mkdir "%testout%"
set "sdk=%~1\Program Files\Windows Kits\10"
cl /nologo /std:c++17 /EHsc /W4 /WX /utf-8 /O2 /I"%sdk%\Include\10.0.26100.0\um" /I"%sdk%\Include\10.0.26100.0\shared" /I"%sdk%\Include\10.0.26100.0\ucrt" windows\protocol\tests\EncoderWorkerTests.cpp /Fo"%testout%\EncoderWorkerTests.obj" /Fe"%testout%\EncoderWorkerTests.exe" /link /LIBPATH:"%sdk%\Lib\10.0.26100.0\um\x64" /LIBPATH:"%sdk%\Lib\10.0.26100.0\ucrt\x64"
set "result=%errorlevel%"
popd
exit /b %result%
