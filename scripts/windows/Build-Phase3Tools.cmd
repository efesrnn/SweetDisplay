@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
msbuild windows\tools\FrameProbe.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
if errorlevel 1 goto failed
msbuild windows\tools\GpuFramePattern.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
if errorlevel 1 goto failed
msbuild windows\host\SweetDisplayHost.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
if errorlevel 1 goto failed
popd
exit /b 0
:failed
popd
exit /b 1
