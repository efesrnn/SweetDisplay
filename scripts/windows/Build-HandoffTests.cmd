@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
msbuild windows\host\tests\FrameHandoffTests.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
set result=%errorlevel%
popd
exit /b %result%
