#requires -Version 7.4
param([string]$SdkPath,[string]$JavaHome)
$ErrorActionPreference='Stop'
$root=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$rootFull=[IO.Path]::GetFullPath($root).TrimEnd([IO.Path]::DirectorySeparatorChar)
if(!$SdkPath){$SdkPath=if($env:ANDROID_SDK_ROOT){$env:ANDROID_SDK_ROOT}else{Join-Path $env:LOCALAPPDATA 'Android/Sdk'}}
if(!$JavaHome){
 $localJdk=Get-ChildItem -LiteralPath (Join-Path $env:USERPROFILE '.gradle/jdks') -Recurse -Filter javac.exe -File -ErrorAction SilentlyContinue|Select-Object -First 1
 if($localJdk){$JavaHome=Split-Path (Split-Path $localJdk.FullName -Parent) -Parent}else{$system=Get-Command javac.exe -CommandType Application -ErrorAction Stop;$JavaHome=Split-Path (Split-Path $system.Source -Parent) -Parent}
}
$source=Join-Path $root 'device/android-receiver'
$local=Join-Path $root '.local/device-phase2c0'
$build=Join-Path $local 'receiver-build'
$buildFull=[IO.Path]::GetFullPath($build)
if(!$buildFull.StartsWith($rootFull+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Build path escaped repository'}
$classes=Join-Path $build classes;$dex=Join-Path $build dex
Remove-Item -LiteralPath $build -Recurse -Force -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Path $classes,$dex -Force|Out-Null
$tools=Join-Path $SdkPath 'build-tools/35.0.0';$jar=Join-Path $SdkPath 'platforms/android-33/android.jar'
$aapt=Join-Path $tools aapt.exe;$d8=Join-Path $tools d8.bat;$zipalign=Join-Path $tools zipalign.exe;$apksigner=Join-Path $tools apksigner.bat
$javac=Join-Path $JavaHome 'bin/javac.exe';$keytool=Join-Path $JavaHome 'bin/keytool.exe'
foreach($required in @($jar,$aapt,$d8,$zipalign,$apksigner,$javac,$keytool)){if(!(Test-Path -LiteralPath $required)){throw "Missing: $required"}}
$unaligned=Join-Path $build receiver-unaligned.apk;$unsigned=Join-Path $build receiver-unsigned.apk;$apk=Join-Path $build SweetDisplayReceiver.apk
& $aapt package -f -M (Join-Path $source AndroidManifest.xml) -I $jar -F $unaligned
if($LASTEXITCODE){throw 'aapt failed'}
$javaFiles=@(Get-ChildItem -LiteralPath (Join-Path $source src) -Recurse -Filter '*.java' -File|ForEach-Object FullName)
& $javac --release 8 -classpath $jar -d $classes @javaFiles
if($LASTEXITCODE){throw 'javac failed'}
$env:JAVA_HOME=$JavaHome
$classFiles=@(Get-ChildItem -LiteralPath $classes -Recurse -Filter '*.class' -File|ForEach-Object FullName)
& $d8 --lib $jar --output $dex @classFiles
if($LASTEXITCODE){throw 'd8 failed'}
Copy-Item -LiteralPath $unaligned -Destination $unsigned
Push-Location $dex
try{& $aapt add $unsigned classes.dex}finally{Pop-Location}
if($LASTEXITCODE){throw 'aapt add failed'}
& $zipalign -f -p 4 $unsigned $apk
if($LASTEXITCODE){throw 'zipalign failed'}
$keystore=Join-Path $local receiver-debug.keystore
if(!(Test-Path -LiteralPath $keystore)){
 & $keytool -genkeypair -keystore $keystore -storepass android -alias receiver -keypass android -dname 'CN=SweetDisplay Development Receiver,O=Local Development,C=TR' -keyalg RSA -keysize 2048 -validity 365 -noprompt
 if($LASTEXITCODE){throw 'keytool failed'}
}
& $apksigner sign --ks $keystore --ks-pass pass:android --key-pass pass:android $apk
if($LASTEXITCODE){throw 'apksigner failed'}
& $apksigner verify --verbose $apk
if($LASTEXITCODE){throw 'APK verification failed'}
[pscustomobject]@{Apk=$apk;Bytes=(Get-Item $apk).Length;Sha256=(Get-FileHash $apk -Algorithm SHA256).Hash.ToLowerInvariant();Permissions='android.permission.INTERNET (normal)';Port=48231}|ConvertTo-Json
