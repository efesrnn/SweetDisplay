@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
if not exist "out\diagnostics" mkdir "out\diagnostics"
msbuild windows\tools\FrameProbe.vcxproj /p:Configuration=Release /p:Platform=x64 /p:SignMode=Off /nologo /v:minimal
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
