#requires -Version 7.4
<# DEVICE PHASE 2C-PERF2 bounded degradation-triggered flight recorder.
   It requires an already-running ADB server and never restarts it. Only the
   Host/Windows-health worker is elevated; ADB remains in its existing context. #>
param(
 [Parameter(Mandatory=$true)][string]$RunName,
 [ValidateRange(120,1800)][int]$Seconds=1800,
 [ValidateRange(1024,65535)][int]$Port=48231,
 [string]$AdbPath=(Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe'),
 [ValidateSet('perf2','perf3')][string]$BuildFlavor='perf2',
 [switch]$TouchRegression
)
$ErrorActionPreference='Stop'
[Threading.Thread]::CurrentThread.CurrentCulture=[Globalization.CultureInfo]::InvariantCulture
if($RunName-notmatch'^[a-zA-Z0-9-]+$'){throw 'Invalid run name'}
if(!(Test-Path -LiteralPath $AdbPath -PathType Leaf)){throw 'ADB executable not found'}
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$evidencePhase=if($BuildFlavor-eq'perf3'){'device-phase2cperf3'}else{'device-phase2cperf2'}
$run=Join-Path $repo ('docs\evidence\private\'+$evidencePhase+'\'+$RunName)
if(Test-Path -LiteralPath $run){throw 'Preserve evidence: use a new run name'}
$hostDir=Join-Path $run 'host';New-Item -ItemType Directory -Path $hostDir -Force|Out-Null
$abortFile=Join-Path $run 'abort.request';$stopFile=Join-Path $run 'normal-host-stop.request';$hostStdout=Join-Path $hostDir 'stdout.txt'
 $result=[ordered]@{Outcome='RUNNING';Phase=$BuildFlavor.ToUpperInvariant();StartedUtc=[datetime]::UtcNow.ToString('o');RequestedSeconds=$Seconds;Width=2400;Height=1080;Rate=60;Bitrate=30000000;Mode='normal';TouchRegression=[bool]$TouchRegression;DetectorWindowSeconds=10.0;DetectorMinimumSourceFps=45.0;DetectorRatioThreshold=0.75;PreTargetSeconds=30;PostTargetSeconds=60;Port=$Port;AdbPidBefore=$null;AdbPidAfter=$null;AdbStateChanged=$null;UsbBefore=$null;UsbAfter=$null;Triggered=$null;TriggerSnapshotCaptured=$false;HostExitCode=$null;PatternExitCode=$null;OperatorAbortFile=$abortFile;OperatorAborted=$false;Error=$null}
function Save-Result{$result|ConvertTo-Json -Depth 15|Set-Content -LiteralPath (Join-Path $run 'controller.json') -Encoding utf8}
$script:InitialAdbPid=0
function Confirm-AdbServer{
 $processes=@(Get-Process -Name adb -ErrorAction SilentlyContinue)
 if($processes.Count-ne1){throw 'Existing unique ADB server is required; PERF2 will not start or restart it'}
 if($script:InitialAdbPid-and$processes[0].Id-ne$script:InitialAdbPid){throw 'ADB server identity changed; refusing any command that could start another server'}
 return $processes[0]
}
function Invoke-Adb([switch]$AllowNotFound,[Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments){
 $null=Confirm-AdbServer
 $start=[Diagnostics.ProcessStartInfo]::new();$start.FileName=$AdbPath;$start.UseShellExecute=$false;$start.CreateNoWindow=$true;$start.RedirectStandardOutput=$true;$start.RedirectStandardError=$true
 $start.ArgumentList.Add('-H');$start.ArgumentList.Add('127.0.0.1');$start.ArgumentList.Add('-P');$start.ArgumentList.Add('5037')
 foreach($argument in $Arguments){$start.ArgumentList.Add($argument)}
 $process=[Diagnostics.Process]::new();$process.StartInfo=$start;if(!$process.Start()){throw 'adb client start failed'}
 $stdout=$process.StandardOutput.ReadToEndAsync();$stderr=$process.StandardError.ReadToEndAsync()
 if(!$process.WaitForExit(15000)){try{$process.Kill()}catch{};throw('adb client deadline without server recovery: '+($Arguments-join' '))}
 $text=($stdout.GetAwaiter().GetResult()+$stderr.GetAwaiter().GetResult()).TrimEnd();if($process.ExitCode-ne0-and!($AllowNotFound-and$process.ExitCode-eq1)){throw('adb client failed without server recovery: '+$text)}
 $null=Confirm-AdbServer
 if(!$text){return @()};return @($text-split"`r?`n")
}
function Get-AdbSnapshot([string]$Name){
 $adb=Confirm-AdbServer;$listeners=@(Get-NetTCPConnection -State Listen -LocalPort 5037 -ErrorAction SilentlyContinue|Select-Object LocalAddress,LocalPort,OwningProcess)
 if($listeners.Count-ne1-or$listeners[0].OwningProcess-ne$script:InitialAdbPid){throw 'ADB server listener identity changed'}
 $snapshot=[ordered]@{Utc=[datetime]::UtcNow.ToString('o');Pid=$adb.Id;StartTimeUtc=$adb.StartTime.ToUniversalTime().ToString('o');Listener=$listeners;State=(Invoke-Adb -Arguments @('get-state')|Select-Object -First 1);UsbConfig=(Invoke-Adb -Arguments @('shell','getprop','sys.usb.config')|Select-Object -First 1).Trim();UsbState=(Invoke-Adb -Arguments @('shell','getprop','sys.usb.state')|Select-Object -First 1).Trim();ReceiverPid=((Invoke-Adb -AllowNotFound -Arguments @('shell','pidof','com.sweetdisplay.receiver')|Select-Object -First 1)+'').Trim();Forwards=@(Invoke-Adb -Arguments @('forward','--list'))}
 $snapshot|ConvertTo-Json -Depth 8|Set-Content -LiteralPath (Join-Path $run ('adb-'+$Name+'.json')) -Encoding utf8
 return $snapshot
}
function Add-ResourceSample([string]$Role,[int]$Id,[double]$Elapsed){$process=Get-Process -Id $Id -ErrorAction SilentlyContinue;if(!$process){return};[pscustomobject]@{ControllerUtc=[datetime]::UtcNow.ToString('o');ControllerTick=[Diagnostics.Stopwatch]::GetTimestamp();ElapsedSeconds=$Elapsed;Role=$Role;ProcessId=$Id;CpuSeconds=$process.TotalProcessorTime.TotalSeconds;PrivateBytes=$process.PrivateMemorySize64;WorkingSetBytes=$process.WorkingSet64;Handles=$process.HandleCount;Threads=$process.Threads.Count}|Export-Csv -LiteralPath (Join-Path $run 'windows-resources.csv') -Append -NoTypeInformation -Encoding utf8}
function Get-AndroidCounters([string]$Text){
 $line=@($Text-split"`r?`n"|Where-Object{$_-match'METRICS event='}|Select-Object -Last 1);if(!$line){throw 'Android METRICS snapshot missing'}
 $values=[ordered]@{};foreach($match in [regex]::Matches($line[0],'(?<key>[A-Za-z][A-Za-z0-9]+)=(?<value>[^\s]+)')){$values[$match.Groups['key'].Value]=$match.Groups['value'].Value}
 foreach($key in @('cleanDrains','protocolErrors','crcErrors','sequenceErrors','sessionErrors','queueOverflows','decoderOutputs','decoderErrors')){if(!$values.Contains($key)){throw('Android metric missing: '+$key)}}
 return $values
}
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class SweetDisplayPerf2Native {
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint pid);
 [DllImport("user32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$pattern=$null;$elevatedWorker=$null;$hostPid=0;$forwardCreated=$false;$receiverStarted=$false;$driverPid=0;$helperPid=0;$androidBefore=$null;$triggerSnapshot=$null
try{
 Save-Result
 $adbProcess=Confirm-AdbServer;$script:InitialAdbPid=$adbProcess.Id;$result.AdbPidBefore=$script:InitialAdbPid
 $pre=Get-AdbSnapshot 'before';$result.UsbBefore=$pre.UsbConfig
 if($pre.State-ne'device'-or$pre.UsbConfig-ne'mtp,adb'-or$pre.UsbState-ne'mtp,adb'){throw 'Existing ADB/USB baseline is not ready'}
 $hostExe=Join-Path $repo ('out\transport-host-'+$BuildFlavor+'\SweetDisplayHost.exe');$patternExe=Join-Path $repo 'out\gpu-pattern\SweetDisplayGpuPattern.exe'
 foreach($required in @($hostExe,$patternExe)){if(!(Test-Path -LiteralPath $required -PathType Leaf)){throw("Missing executable: "+$required)}}
 $drive=Get-PSDrive -Name ([IO.Path]::GetPathRoot($repo).Substring(0,1));if($drive.Free-lt4GB){throw 'At least 4 GiB free space is required'}
 $helpers=@(Get-Process SweetDisplayDevice -ErrorAction Stop);if($helpers.Count-ne1){throw 'Unique SweetDisplayDevice helper required'};$helperPid=$helpers[0].Id
 if(!(Invoke-Adb -Arguments @('shell','pm','list','packages','com.sweetdisplay.receiver')|Select-String 'package:com.sweetdisplay.receiver')){throw 'Android receiver is not installed'}
 Invoke-Adb -Arguments @('forward',('tcp:'+$Port),('tcp:'+$Port))|Out-Null;$forwardCreated=$true
 Invoke-Adb -Arguments @('shell','am','start','-W','-n','com.sweetdisplay.receiver/.MainActivity','--ez','ack_only','false')|Out-Null;$receiverStarted=$true
 Start-Sleep -Seconds 2
 $modeLog=(Invoke-Adb -Arguments @('logcat','-d','-v','epoch','SWDPRX:I','*:S'))-join"`n";if($modeLog-notmatch'MODE ackOnly=false'){throw 'Normal Android receiver mode confirmation missing'}
 $androidBefore=Get-AndroidCounters $modeLog;$result.AndroidBefore=$androidBefore
 $nonce='{0:X8}'-f(Get-Random -Minimum 1 -Maximum ([int]::MaxValue));$result.Nonce=$nonce
  $hostArgs='--output "'+$hostDir+'" --seconds '+$Seconds+' --stop-file "'+$stopFile+'" --first-fail --classified --nonce '+$nonce+' --encode --encode-width 2400 --encode-height 1080 --encode-fps 60 --encode-bitrate 30000000 --encode-uncapped --transport-port '+$Port+' --flight-recorder --reduced-evidence'
  if($TouchRegression){$hostArgs+=' --touch-mode inject'}
 [ordered]@{HostExe=$hostExe;HostArguments=$hostArgs;Seconds=$Seconds;StopFile=$stopFile}|ConvertTo-Json|Set-Content -LiteralPath (Join-Path $run 'elevated-host-command.json') -Encoding utf8
 $workerScript=Join-Path $PSScriptRoot 'Invoke-Perf2ElevatedHost.ps1';$workerCommand="& '$workerScript' -RunDirectory '$run'";$workerEncoded=[Convert]::ToBase64String([Text.Encoding]::Unicode.GetBytes($workerCommand));$workerPwsh=Join-Path $PSHOME 'pwsh.exe'
 $elevatedWorker=Start-Process -FilePath $workerPwsh -ArgumentList @('-NoProfile','-EncodedCommand',$workerEncoded) -WorkingDirectory $repo -Verb RunAs -WindowStyle Hidden -PassThru
 $workerReady=Join-Path $run 'elevated-worker-ready.json';$workerReadyDeadline=[datetime]::UtcNow.AddSeconds(60)
 while(!(Test-Path -LiteralPath $workerReady)){if($elevatedWorker.HasExited){$workerResultPath=Join-Path $run 'elevated-host-result.json';$workerError=if(Test-Path -LiteralPath $workerResultPath){(Get-Content -Raw $workerResultPath|ConvertFrom-Json).Error}else{'elevated worker exited before readiness'};throw('Elevated Host worker failed: '+$workerError)};if([datetime]::UtcNow-gt$workerReadyDeadline){throw 'Elevated Host worker readiness deadline'};Start-Sleep -Milliseconds 100}
 $workerReadyData=Get-Content -Raw -LiteralPath $workerReady|ConvertFrom-Json;$driverPid=[int]$workerReadyData.DriverPid
  $patternMode=if($TouchRegression){'touch'}else{'observe'}
  $pattern=Start-Process -FilePath $patternExe -ArgumentList ('"'+$run+'" '+$nonce+' '+($Seconds+120)+' 0 1 '+$patternMode) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'pattern-stdout.txt') -RedirectStandardError (Join-Path $run 'pattern-stderr.txt')
 Start-Sleep -Seconds 2;if($pattern.HasExited){throw 'Pattern failed during startup'}
 'pattern-ready'|Set-Content -LiteralPath (Join-Path $run 'pattern-ready.request') -Encoding ascii
 $workerStart=Join-Path $run 'elevated-host-start.json';$workerStartDeadline=[datetime]::UtcNow.AddSeconds(60)
 while(!(Test-Path -LiteralPath $workerStart)){if($elevatedWorker.HasExited){$workerResultPath=Join-Path $run 'elevated-host-result.json';$workerError=if(Test-Path -LiteralPath $workerResultPath){(Get-Content -Raw $workerResultPath|ConvertFrom-Json).Error}else{'elevated worker exited before status'};throw('Elevated Host worker failed: '+$workerError)};if([datetime]::UtcNow-gt$workerStartDeadline){throw 'Elevated Host worker startup deadline'};Start-Sleep -Milliseconds 100}
 $workerStartData=Get-Content -Raw -LiteralPath $workerStart|ConvertFrom-Json;$hostPid=[int]$workerStartData.HostPid
 $started=[datetime]::UtcNow;$deadline=$started.AddSeconds($Seconds+180);$nextSample=$started;$nextProgress=60
  Write-Host (($BuildFlavor.ToUpperInvariant())+' flight recorder armed. Safe abort file: '+$abortFile) -ForegroundColor Green
  if($TouchRegression){Write-Host 'TOUCH: Telefonda tek parmakla bas, surukle ve birak.' -ForegroundColor Yellow}
 while(!$elevatedWorker.WaitForExit(100)){
  $elapsed=([datetime]::UtcNow-$started).TotalSeconds;if($pattern.HasExited){throw 'Pattern exited while Host was active'};if([datetime]::UtcNow-gt$deadline){throw 'Host exceeded bounded deadline'}
  if(Test-Path -LiteralPath $abortFile){$result.OperatorAborted=$true;if(!(Test-Path -LiteralPath $stopFile)){'operator-abort'|Set-Content -LiteralPath $stopFile -Encoding ascii}}
  if($elapsed-ge$nextProgress){Write-Host ('PERF2 elapsed: '+[int]$elapsed+' / '+$Seconds+' seconds') -ForegroundColor Cyan;$nextProgress+=60}
  if([datetime]::UtcNow-ge$nextSample){
   try{Add-ResourceSample 'Host' $hostPid $elapsed}catch{};Add-ResourceSample 'Pattern' $pattern.Id $elapsed;try{Add-ResourceSample 'Driver' $driverPid $elapsed}catch{};Add-ResourceSample 'Helper' $helperPid $elapsed
   if(!$triggerSnapshot-and(Test-Path -LiteralPath $hostStdout)-and((Get-Content -LiteralPath $hostStdout -Tail 20)-match'PERF2 TRIGGER')){$triggerSnapshot=Get-AdbSnapshot 'trigger';$result.TriggerSnapshotCaptured=$true;Write-Host 'PERF2 trigger observed; ADB state captured read-only. Post-trigger capture continues.' -ForegroundColor Yellow}
   $nextSample=[datetime]::UtcNow.AddSeconds(5)
  }
 }
 $workerResult=Get-Content -Raw -LiteralPath (Join-Path $run 'elevated-host-result.json')|ConvertFrom-Json;$result.HostExitCode=$workerResult.HostExitCode;if($workerResult.Outcome-ne'PASS'-or$workerResult.HostExitCode-ne0){throw('Elevated Host worker failed: '+$workerResult.Error+' '+$workerResult.HealthError)}
 if($result.OperatorAborted){$result.Outcome='ABORTED'}else{
  foreach($required in @('session-1-result.json','encode-result.json','transport-result.json','classification-result.json','flight-recorder-meta.json')){if(!(Test-Path -LiteralPath (Join-Path $hostDir $required))){throw('Missing Host result: '+$required)}}
  $source=Get-Content -Raw (Join-Path $hostDir 'session-1-result.json' )|ConvertFrom-Json;$encode=Get-Content -Raw (Join-Path $hostDir 'encode-result.json')|ConvertFrom-Json;$transport=Get-Content -Raw (Join-Path $hostDir 'transport-result.json')|ConvertFrom-Json;$classification=Get-Content -Raw (Join-Path $hostDir 'classification-result.json')|ConvertFrom-Json;$flight=Get-Content -Raw (Join-Path $hostDir 'flight-recorder-meta.json')|ConvertFrom-Json
  if(!$source.clean_disconnect-or!$encode.hardware_only-or$encode.software_fallback-or!$encode.drained-or$transport.protocol_errors-ne0-or$transport.pending-ne0-or$transport.queue_peak-gt3-or$classification.B-or$classification.C-or$classification.D-or$classification.E){throw 'PERF2 invariant failed'}
  if($BuildFlavor-eq'perf3'-and(!$encode.mft_single_owner-or$encode.host_services_mft_events-or$encode.input_queue_capacity-ne4-or$encode.input_queue_peak-gt4-or$encode.input_queue_pending-ne0-or$encode.output_queue_capacity-ne3)){throw 'PERF3 encoder ownership/queue invariant failed'}
  $result.Source=$source;$result.Encoder=$encode;$result.Transport=$transport;$result.Classification=$classification;$result.FlightRecorder=$flight;$result.Triggered=[bool]$flight.triggered;$result.HostSourceRatio=if([double]$source.source_fps){[double]$source.fps/[double]$source.source_fps}else{0}
  if($TouchRegression){
   $touchPath=Join-Path $hostDir 'touch-events.csv';$targetPath=Join-Path $run 'touch-target-events.csv'
   if(!(Test-Path -LiteralPath $touchPath)-or!(Test-Path -LiteralPath $targetPath)){throw 'Touch regression evidence missing'}
   $touch=@(Import-Csv -LiteralPath $touchPath|Where-Object event -in 'DOWN','MOVE','UP','CANCEL');$target=@(Import-Csv -LiteralPath $targetPath|Where-Object event -in 'DOWN','MOVE','UP')
   foreach($requiredEvent in @('DOWN','MOVE','UP')){if(!($touch.event-contains$requiredEvent)-or!($target.event-contains$requiredEvent)){throw 'Touch regression requires DOWN, MOVE and UP'}}
   $hostActive=[Collections.Generic.HashSet[string]]::new();foreach($event in $touch){$id=[string]$event.contact;if($event.event-eq'DOWN'){$null=$hostActive.Add($id)}elseif($event.event-in'UP','CANCEL'){$null=$hostActive.Remove($id)}}
   $targetActive=[Collections.Generic.HashSet[string]]::new();foreach($event in $target){$id=[string]$event.pointer_id;if($event.event-eq'DOWN'){$null=$targetActive.Add($id)}elseif($event.event-eq'UP'){$null=$targetActive.Remove($id)}}
   if($hostActive.Count-or$targetActive.Count){throw 'Touch regression ended with active contacts'}
   $result.Touch=[ordered]@{HostEvents=$touch.Count;TargetEvents=$target.Count;HostFinalActive=$hostActive.Count;TargetFinalActive=$targetActive.Count}
  }
  if($flight.triggered-and!$triggerSnapshot){$triggerSnapshot=Get-AdbSnapshot 'trigger-late';$result.TriggerSnapshotCaptured=$true}
  if($flight.triggered){if(!(Test-Path -LiteralPath (Join-Path $hostDir 'flight-recorder.bin'))){throw 'Triggered run has no trace'};if([double]$flight.post_coverage_seconds-lt59){throw 'Post-trigger capture is short'};$result.Outcome='PASS_TRIGGERED_PENDING_ANALYSIS'}
  else{if(Test-Path -LiteralPath (Join-Path $hostDir 'flight-recorder.bin')){throw 'No-trigger run retained a raw trace'};if([double]$source.seconds-lt($Seconds-2)){throw 'No-trigger observation ended early'};$result.Outcome='PASS_NO_TRIGGER_PENDING_ANALYSIS'}
 }
}catch{$result.Outcome='ERROR';$result.Error=$_.Exception.Message}
finally{
 if($elevatedWorker-and!$elevatedWorker.HasExited){'controller-cleanup'|Set-Content -LiteralPath (Join-Path $run 'elevated-worker-stop.request') -Encoding ascii;if(!(Test-Path -LiteralPath $stopFile)){'cleanup-stop'|Set-Content -LiteralPath $stopFile -Encoding ascii};if(!$elevatedWorker.WaitForExit(30000)){$result.HostForcedStop=$true;$result.Outcome='ERROR'}}
 try{
  if($receiverStarted){$androidText=(Invoke-Adb -Arguments @('logcat','-d','-v','epoch','SWDPRX:I','*:S'))-join"`n";$androidText|Set-Content -LiteralPath (Join-Path $run 'android-swdprx.log') -Encoding utf8;$androidAfter=Get-AndroidCounters $androidText;$result.AndroidAfter=$androidAfter;if($androidBefore){foreach($key in @('protocolErrors','crcErrors','sequenceErrors','sessionErrors','queueOverflows','decoderErrors')){if(([long]$androidAfter[$key]-[long]$androidBefore[$key])-ne0){throw('Android counter delta nonzero: '+$key)}};if(([long]$androidAfter.cleanDrains-[long]$androidBefore.cleanDrains)-ne1){throw 'Android clean-drain delta is not one'};if(([long]$androidAfter.decoderOutputs-[long]$androidBefore.decoderOutputs)-le0){throw 'Normal receiver produced no decoder output'}}}
  $null=Get-AdbSnapshot 'capture-complete'
 }catch{$result.CleanupError=$_.Exception.Message;$result.Outcome='ERROR'}
 if($forwardCreated){try{Invoke-Adb -Arguments @('forward','--remove',('tcp:'+$Port))|Out-Null}catch{$result.ForwardCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}}
 if($receiverStarted){try{Invoke-Adb -Arguments @('shell','am','force-stop','com.sweetdisplay.receiver')|Out-Null}catch{$result.ReceiverCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}}
 if($pattern-and!$pattern.HasExited){$closed=$false;$generation=Join-Path $hostDir 'resource-generation.txt';if(Test-Path $generation){$identity=[IO.File]::ReadAllText($generation);if($identity-match'pattern_hwnd=(\d+)'){$window=[IntPtr]::new([long]$Matches[1]);[uint32]$owner=0;[SweetDisplayPerf2Native]::GetWindowThreadProcessId($window,[ref]$owner)|Out-Null;if($owner-eq$pattern.Id){$closed=[SweetDisplayPerf2Native]::PostMessage($window,0x10,[IntPtr]::Zero,[IntPtr]::Zero)}}};if(!$closed){$closed=$pattern.CloseMainWindow()};if(!$pattern.WaitForExit(5000)){$pattern.Kill();$result.PatternForcedStop=$true;$result.Outcome='ERROR'}}
 if($pattern-and$pattern.HasExited){$result.PatternExitCode=$pattern.ExitCode;if($pattern.ExitCode-ne0){$result.Outcome='ERROR'}}
 try{$beforeHealth=Get-Content -Raw -LiteralPath (Join-Path $run 'before-health.json')|ConvertFrom-Json;$afterHealth=Get-Content -Raw -LiteralPath (Join-Path $run 'after-health.json')|ConvertFrom-Json;if($beforeHealth.Boot-ne$afterHealth.Boot-or$beforeHealth.Hvci-ne$afterHealth.Hvci){throw 'Windows boot/security changed'};$adbAfter=Get-AdbSnapshot 'after-cleanup';$result.AdbPidAfter=$adbAfter.Pid;$result.UsbAfter=$adbAfter.UsbConfig;$result.AdbStateChanged=($result.AdbPidBefore-ne$result.AdbPidAfter);if($result.AdbStateChanged){throw 'ADB server identity changed'};if($result.UsbAfter-ne'mtp,adb'-or$adbAfter.UsbState-ne'mtp,adb'){throw 'USB composition changed'}}catch{$result.HealthCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}
 $result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save-Result
}
if($result.Outcome-notin@('PASS_TRIGGERED_PENDING_ANALYSIS','PASS_NO_TRIGGER_PENDING_ANALYSIS')){throw($result.Outcome+': '+$result.Error+' '+$result.CleanupError+' '+$result.HealthCleanupError)}
& (Join-Path $PSScriptRoot 'Analyze-AndroidFlightRecorder.ps1') -RunName $RunName -EvidencePhase $evidencePhase
if($LASTEXITCODE){throw 'Flight-recorder analysis failed'}
$result.Outcome=if($result.Triggered){'PASS_TRIGGERED_ANALYZED'}else{'PASS_NO_TRIGGER_ANALYZED'};Save-Result
Write-Output ($result.Outcome+': '+$RunName)
