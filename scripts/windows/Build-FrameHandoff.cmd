@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
msbuild windows\host\SweetDisplayHost.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
if errorlevel 1 goto failed
if /i "%~2"=="host-only" goto succeeded
msbuild windows\host\tests\FrameHandoffTests.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
if errorlevel 1 goto failed
out\host-tests\FrameHandoffTests.exe
if errorlevel 1 goto failed
msbuild windows\driver\SweetDisplayDriver\SweetDisplayDriver.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /p:SignMode=Off /p:Inf2CatUseLocalTime=true /nologo /v:minimal
if errorlevel 1 goto failed
:succeeded
popd
exit /b 0
:failed
popd
exit /b 1
