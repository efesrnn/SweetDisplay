@echo off
setlocal
if "%~1"=="" exit /b 2
call "%~1\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
msbuild windows\driver\SweetDisplayDriver\SweetDisplayDriver.vcxproj /t:Build /p:Configuration=Debug /p:Platform=x64 /p:SignMode=Off /p:Inf2CatUseLocalTime=true /nologo /v:minimal
set "RESULT=%errorlevel%"
popd
exit /b %RESULT%
