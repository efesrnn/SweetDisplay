@echo off
setlocal
if "%~1"=="" (
  echo Usage: Build-SweetDisplay.cmd EWDK_ROOT
  exit /b 2
)
set "SWEET_EWDK=%~f1"
if not exist "%SWEET_EWDK%\BuildEnv\SetupBuildEnv.cmd" exit /b 2
call "%SWEET_EWDK%\BuildEnv\SetupBuildEnv.cmd" amd64
@echo off
if errorlevel 1 exit /b 1
pushd "%~dp0..\.."
if errorlevel 1 exit /b 1
if not exist "docs\evidence\private" mkdir "docs\evidence\private"
if not exist "out\windows\monitor" mkdir "out\windows\monitor"
set "SWEET_KIT=%SWEET_EWDK%\Program Files\Windows Kits\10"
if not exist "%SWEET_KIT%\Tools\10.0.26100.0\x64\infverif.exe" goto fail
msbuild windows\driver\SweetDisplay.sln /t:Build /p:Configuration=Debug /p:Platform=x64 /p:SignMode=Off /nologo /v:minimal /fl "/flp:logfile=docs\evidence\private\sweetdisplay-last-build.log;verbosity=normal"
if errorlevel 1 goto fail
msbuild windows\driver\tests\MonitorConfigTests.vcxproj /p:Configuration=Debug /p:Platform=x64 /p:SignMode=Off /nologo /v:minimal
if errorlevel 1 goto fail
out\windows\x64\Debug\MonitorConfigTests\bin\MonitorConfigTests.exe
if errorlevel 1 goto fail
copy /y "windows\driver\SweetDisplayMonitor\SweetDisplayMonitor.inf" "out\windows\monitor\SweetDisplayMonitor.inf" >nul
if errorlevel 1 goto fail
"%SWEET_KIT%\Tools\10.0.26100.0\x64\infverif.exe" /w "out\windows\monitor\SweetDisplayMonitor.inf"
if errorlevel 1 goto fail
"%SWEET_KIT%\bin\10.0.26100.0\x86\Inf2Cat.exe" /driver:out\windows\monitor /os:10_GE_X64 /uselocaltime
if errorlevel 1 goto fail
echo PASS: build, mode tests, monitor INF validation and catalogs. Packages remain unsigned; nothing installed.
popd
exit /b 0
:fail
echo FAILED: see output above. No installation attempted.
popd
exit /b 1
