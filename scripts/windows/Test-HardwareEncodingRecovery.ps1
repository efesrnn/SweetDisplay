# Run only after a verified sustained uncapped run. No installation or policy changes.
param([Parameter(Mandatory=$true)][string]$SustainedEvidenceDirectory,
 [Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunPrefix,
 [string]$VerifiedCleanEvidenceDirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$source=(Resolve-Path -LiteralPath $SustainedEvidenceDirectory).Path
function Json($p){[IO.File]::ReadAllText($p)|ConvertFrom-Json}
function Require($ok,$message){if(!$ok){throw $message}}
Require ((Json (Join-Path $source 'encoding-verification.json')).Outcome -eq 'PASS_ENCODE_DECODE') 'Sustained decode/integrity gate missing'
$strict=Json (Join-Path $source 'uncapped-acceptance.json')
$review=Json (Join-Path $source 'sustained-review.json')
# The latest owner request permits explicitly accounted transient admission drops.
# Preserve the separate all-Host verdict; it must never be relabelled as a pass.
# A bounded-admission pass requires an independent, evidence-bound manual review.
$bounded=$review.Outcome -eq 'PASS_SUSTAINED_REVIEW' -and $review.AcceptanceBasis -eq 'LATEST_OWNER_BOUNDED_ADMISSION' -and $review.ExactAccounting -and $review.NoSustainedBacklog -and $review.ResourceReviewPassed -and $review.ContentIntegrityPassed -and $review.StrictAllHostOutcome -eq $strict.Outcome
Require ($strict.Outcome -eq 'PASS_ALL_UNIQUE_HOST_FRAMES' -or $bounded) 'Sustained admission/accounting review missing'
if($bounded){
 Require (@($review.EvidenceHashes).Count -ge 6) 'Missing review evidence binding'
 foreach($file in $review.EvidenceHashes){Require ((Get-FileHash -LiteralPath (Join-Path $source $file.RelativePath) -Algorithm SHA256).Hash -eq $file.Hash) 'Reviewed sustained evidence changed'}
}
Require ((Json (Join-Path $source 'host/session-1-result.json')).seconds -ge 1800) 'Sustained duration below1800'
Require ($review.Outcome -eq 'PASS_SUSTAINED_REVIEW') 'Sustained resource/latency review missing'
$verifiedClean=$null
if($VerifiedCleanEvidenceDirectory){
 $verifiedClean=(Resolve-Path -LiteralPath $VerifiedCleanEvidenceDirectory).Path
 $cleanVerification=Json (Join-Path $verifiedClean 'encoding-verification.json')
 $cleanExecution=Json (Join-Path $verifiedClean 'execution.json')
 Require ($cleanVerification.Outcome -eq 'PASS_ENCODE_DECODE' -and @($cleanVerification.Sessions).Count -eq 2 -and @($cleanVerification.Sessions|Where-Object Session -eq 'reconnect').Count -eq 1 -and $cleanExecution.Outcome -eq 'PASS_CLASSIFIED' -and $cleanExecution.ReconnectExitCode -eq 0 -and $cleanExecution.PatternExitCode -eq 0) 'Existing clean recovery not verified'
 foreach($s in @('host','reconnect')){
  $ch=Json (Join-Path $verifiedClean ($s+'/session-1-result.json'));$ce=Json (Join-Path $verifiedClean ($s+'/encode-result.json'))
  Require ($ch.clean_disconnect -and $ch.seconds -ge $(if($s -eq 'host'){20}else{15}) -and $ce.input -eq $ce.accepted -and $ce.accepted -eq $ce.outputs -and !$ce.rate_drops -and !$ce.backpressure_drops -and $ce.clean_shutdown) 'Existing clean recovery accounting/duration'
 }
}
Require ([Security.Principal.WindowsPrincipal]::new([Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) 'Administrator required'
$root=Join-Path $repo 'docs/evidence/private/phase3b'
$dir=Join-Path $root ($RunPrefix+'-controller')
foreach($p in @($dir,(Join-Path $root ($RunPrefix+'-clean')),(Join-Path $root ($RunPrefix+'-after-crash')))){Require (!(Test-Path $p)) 'Preserve existing recovery evidence'}
New-Item -ItemType Directory $dir|Out-Null
$state=@{Outcome='RUNNING';StartedUtc=[datetime]::UtcNow.ToString('o');Steps=@();Error=$null}
function Save{$state|ConvertTo-Json -Depth 8|Set-Content (Join-Path $dir 'execution.json') -Encoding utf8}
function Log($name,$value){$state.Steps+=@{Utc=[datetime]::UtcNow.ToString('o');Name=$name;Value=$value};Save}
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class SweetDisplayRecoveryNative {
 [StructLayout(LayoutKind.Sequential)] public struct CI {public uint Length;public uint Options;}
 [DllImport("ntdll.dll")] public static extern int NtQuerySystemInformation(int c,ref CI i,uint n,out uint r);
 [DllImport("kernel32.dll",SetLastError=true)] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool GetExitCodeProcess(IntPtr h,out uint c);
 [DllImport("kernel32.dll",CharSet=CharSet.Unicode,SetLastError=true)] public static extern IntPtr CreateFileW(string p,uint a,uint s,IntPtr sec,uint c,uint f,IntPtr t);
 [DllImport("kernel32.dll",SetLastError=true)] public static extern bool GetFileSizeEx(IntPtr h,out long size);
 [DllImport("kernel32.dll")] public static extern bool CloseHandle(IntPtr h);
 [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h,out uint p);
 [DllImport("user32.dll")] [return:MarshalAs(UnmanagedType.Bool)] public static extern bool PostMessage(IntPtr h,uint m,IntPtr w,IntPtr l);
}
'@
$helperId=0;$driverId=0;$before=$null;$pattern=$null;$crashHost=$null
function Health($name){
 $ci=[SweetDisplayRecoveryNative+CI]::new();$ci.Length=8;[uint32]$size=0
 $code=[SweetDisplayRecoveryNative]::NtQuerySystemInformation(103,[ref]$ci,8,[ref]$size)
 $nodes=@(Get-PnpDevice -Class Display,Monitor -PresentOnly|Where-Object {$_.FriendlyName -like '*SweetDisplay*' -or $_.InstanceId -match 'SweetDisplay|SWT0001'}|ForEach-Object{[pscustomobject]@{Class=$_.Class;Status=$_.Status;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data;Inf=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_DriverInfPath).Data}})
 $driver=@(Get-CimInstance Win32_Process -Filter "Name='WUDFHost.exe'"|Where-Object CommandLine -match 'SweetDisplayDriverGroup')
 $helpers=@(Get-Process SweetDisplayDevice -ErrorAction Stop)
 Require ($driver.Count -eq 1 -and $helpers.Count -eq 1) 'Unique original driver/helper required'
 $h=@{Utc=[datetime]::UtcNow.ToString('o');SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;CiStatus=$code;CiFlags=$ci.Options;Devices=$nodes;DriverPid=$driver[0].ProcessId;HelperPid=$helpers[0].Id;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o')}
 $h|ConvertTo-Json -Depth 5|Set-Content (Join-Path $dir ($name+'-health.json')) -Encoding utf8
 Require ($h.SecureBoot -and $h.Hvci -eq 1 -and $code -eq 0 -and ($ci.Options -band 1) -ne 0 -and ($ci.Options -band 2) -eq 0 -and ($ci.Options -band 0x400) -ne 0) 'Security failure'
 Require ($nodes.Count -eq 2 -and @($nodes|Where-Object {$_.Status -ne 'OK' -or $_.Problem -ne 0}).Count -eq 0) 'PnP failure'
 if($before){Require ($h.Boot -eq $before.Boot -and $h.CiFlags -eq $before.CiFlags -and $h.DriverPid -eq $driverId -and $h.HelperPid -eq $helperId) 'Driver/helper/boot continuity failure'}
 return $h
}
$powershell=Join-Path $env:SystemRoot 'System32/WindowsPowerShell/v1.0/powershell.exe'
$encoder=Join-Path $repo 'out/encode-host-control/SweetDisplayHost.exe'
$decoder=Join-Path $repo 'out/phase3b/decoder/DecodeH264Evidence.exe'
function ReadSharedText($path){
 # RedirectStandardOutput keeps a writer open. Allow that existing write handle
 # while taking a read-only snapshot; ReadAllText's FileShare.Read conflicts.
 $stream=[IO.FileStream]::new($path,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
 $reader=[IO.StreamReader]::new($stream)
 try{return $reader.ReadToEnd()}finally{$reader.Dispose()}
}
function LiveFileSize($path){
 # Directory entries may retain length0 while the CRT writer is open. Query metadata
 # through a zero-data-access handle; do not break the writer's sharing contract.
 $h=[SweetDisplayRecoveryNative]::CreateFileW($path,0,7,[IntPtr]::Zero,3,0,[IntPtr]::Zero)
 Require ($h -ne [IntPtr]::new(-1)) 'Live evidence metadata open failed'
 try{[long]$size=0;Require ([SweetDisplayRecoveryNative]::GetFileSizeEx($h,[ref]$size)) 'Live evidence size failed';return $size}
 finally{[SweetDisplayRecoveryNative]::CloseHandle($h)|Out-Null}
}
function CompleteRun($name,[bool]$reconnect){
 $args=@('-NoProfile','-File',(Join-Path $PSScriptRoot 'Test-HardwareEncoding.ps1'),'-RunName',$name,'-Seconds','20','-ControlCase','observe','-Classified','-Width','2400','-Height','1080','-Rate','60','-Bitrate','30000000','-UncappedSubmission')
 if($reconnect){$args+='-Reconnect'}
 Log 'Run uncapped recovery session' @{Executable=$powershell;Arguments=$args}
 & $powershell @args 1> (Join-Path $dir ($name+'-live-stdout.txt')) 2> (Join-Path $dir ($name+'-live-stderr.txt'))
 Require ($LASTEXITCODE -eq 0) ('Recovery live failure: '+$name)
 $run=Join-Path $root $name;$sessions=@('host');if($reconnect){$sessions+='reconnect'}
 foreach($session in $sessions){
  $sessionDir=Join-Path $run $session
  & $decoder $sessionDir 2400 1080 1> (Join-Path $sessionDir 'decode-stdout.txt') 2> (Join-Path $sessionDir 'decode-stderr.txt')
  Require ($LASTEXITCODE -eq 0) ('Recovery decode failure: '+$session)
 }
 & (Join-Path $PSScriptRoot 'Verify-HardwareEncoding.ps1') -EvidenceDirectory $run -MinimumSeconds 20 -RequireReconnect:$reconnect | Out-File (Join-Path $run 'verification-stdout.txt') -Encoding utf8
 foreach($session in $sessions){$e=Json (Join-Path $run ($session+'/encode-result.json'));Require ($e.input -eq $e.accepted -and $e.accepted -eq $e.outputs -and !$e.rate_drops -and !$e.backpressure_drops) 'Recovery all-Host frame failure'}
 Log 'Recovery encode/decode/all-Host passed' @{Run=$name;Sessions=$sessions}
 return $run
}
function CloseCrashPattern{
 if(!$pattern -or $pattern.HasExited){return}
 $identity=[IO.File]::ReadAllText((Join-Path $dir 'crash-host/resource-generation.txt'))
 Require ($identity -match 'pattern_hwnd=(\d+)') 'Missing own crash pattern HWND'
 $window=[IntPtr]::new([long]$Matches[1]);[uint32]$pidOwner=0
 [SweetDisplayRecoveryNative]::GetWindowThreadProcessId($window,[ref]$pidOwner)|Out-Null
 Require ($pidOwner -eq $pattern.Id) 'Crash pattern HWND owner mismatch'
 Require ([SweetDisplayRecoveryNative]::PostMessage($window,0x10,[IntPtr]::Zero,[IntPtr]::Zero)) 'Crash pattern close failed'
 Require ($pattern.WaitForExit(5000)) 'Crash pattern close timeout'
 [uint32]$exitCode=0;Require ([SweetDisplayRecoveryNative]::GetExitCodeProcess($patternHandle,[ref]$exitCode)) 'Pattern exit query failed'
 Require ($exitCode -eq 0) 'Crash pattern exit failure'
 Log 'Crash pattern clean close' @{ExitCode=$exitCode}
}
try{
 Save;$before=Health 'before';$driverId=$before.DriverPid;$helperId=$before.HelperPid;Log 'Initial health' $before
 Require ($driverId -eq (Json (Join-Path $source 'execution.json')).DriverPid -and $before.Boot -eq (Json (Join-Path $source 'after-health.json')).Boot) 'Sustained-to-recovery driver/boot continuity'
 if($verifiedClean){
  Require ($cleanExecution.DriverPid -eq $driverId -and (Json (Join-Path $verifiedClean 'after-health.json')).Boot -eq $before.Boot) 'Existing clean-to-recovery continuity'
  Log 'Preserve already verified clean recovery; no repeat' @{Evidence=$verifiedClean;VerificationHash=(Get-FileHash (Join-Path $verifiedClean 'encoding-verification.json')).Hash}
 }else{$cleanRun=CompleteRun ($RunPrefix+'-clean') $true}
 Health 'after-clean'|Out-Null
 $patternDir=Join-Path $dir 'crash-pattern';$crashDir=Join-Path $dir 'crash-host'
 New-Item -ItemType Directory $patternDir,$crashDir|Out-Null
 $nonce='{0:X8}' -f (Get-Random -Minimum 1 -Maximum ([int]::MaxValue))
 $patternArgs='"'+$patternDir+'" '+$nonce+' 120 0 1 observe'
 $pattern=Start-Process (Join-Path $repo 'out/gpu-pattern/SweetDisplayGpuPattern.exe') -ArgumentList $patternArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $patternDir 'stdout.txt') -RedirectStandardError (Join-Path $patternDir 'stderr.txt')
 $patternHandle=$pattern.Handle
 Start-Sleep -Seconds 2;Require (!$pattern.HasExited) 'Crash pattern startup failed'
 $args='--output "'+$crashDir+'" --seconds 60 --classified --nonce '+$nonce+' --encode --encode-width 2400 --encode-height 1080 --encode-fps 60 --encode-bitrate 30000000 --encode-uncapped'
 $crashHost=Start-Process $encoder -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $crashDir 'stdout.txt') -RedirectStandardError (Join-Path $crashDir 'stderr.txt')
 $crashHandle=$crashHost.Handle;Log 'Start own active encoder for crash' @{Pid=$crashHost.Id;Executable=$encoder;Arguments=$args}
 $deadline=[datetime]::UtcNow.AddSeconds(15);$previousBytes=0L;$active=$false
 while([datetime]::UtcNow -lt $deadline){
  Start-Sleep -Milliseconds 250;Require (!$crashHost.HasExited -and !$pattern.HasExited) 'Crash probe exited unexpectedly'
  $stdout=ReadSharedText (Join-Path $crashDir 'stdout.txt')
  $file=Join-Path $crashDir 'access-units.bin';$size=if(Test-Path $file){LiveFileSize $file}else{0}
  $reported=[regex]::Matches($stdout,'Frames=\d+[^\r\n]*LastID=(\d+)')
  if($size -gt 1MB -and $size -gt $previousBytes -and $reported.Count){$lastCrashId=[long]$reported[$reported.Count-1].Groups[1].Value;$active=$true;break};$previousBytes=$size
 }
 Require $active 'No active encoded output before controlled termination'
 $crashIdentity=[IO.File]::ReadAllText((Join-Path $crashDir 'resource-generation.txt'))
 Require ($crashIdentity -match 'driver_epoch=(\d+)') 'Missing crash epoch';$crashEpoch=[long]$Matches[1]
 $crashHost.Kill();Require ($crashHost.WaitForExit(5000)) 'Own crash Host did not terminate'
 [uint32]$crashCode=0;Require ([SweetDisplayRecoveryNative]::GetExitCodeProcess($crashHandle,[ref]$crashCode)) 'Crash exit code unavailable'
 Log 'Forced termination while actual encoder output active' @{Pid=$crashHost.Id;ExitCode=$crashCode;EncodedEvidenceBytesBeforeKill=$size;LastReportedFrameId=$lastCrashId;Epoch=$crashEpoch;Scope='Active encoding verified, instantaneous texture ownership at termination not asserted; unflushed crash-tail files are diagnostic only'}
 Health 'after-crash'|Out-Null
 $absent=Join-Path $dir 'absent-after-crash';New-Item -ItemType Directory $absent|Out-Null
 & (Join-Path $repo 'out/host/SweetDisplayHost.exe') --output $absent --seconds 2 --inspect-seconds 2 1> (Join-Path $absent 'stdout.txt') 2> (Join-Path $absent 'stderr.txt')
 Require ($LASTEXITCODE -eq 0) 'Driver connection not released after Host crash'
 $idle=Json (Join-Path $absent 'session-1-absent.json');Require (!$idle.connected -and $idle.active -and $idle.source_delta -gt 0) 'Driver source did not continue independently'
 Log 'Driver alive and disconnected after crash' $idle
 CloseCrashPattern
 $afterRun=CompleteRun ($RunPrefix+'-after-crash') $false
 $fresh=Json (Join-Path $afterRun 'host/session-1-result.json')
 Require ($fresh.epoch -eq $crashEpoch -and $fresh.first_id -gt $lastCrashId) 'Post-crash driver epoch/frame ordering'
 $freshIdentity=[IO.File]::ReadAllText((Join-Path $afterRun 'host/resource-generation.txt'))
 $oldNames=@([regex]::Matches($crashIdentity,'shared_resource=(\S+)')|ForEach-Object{$_.Groups[1].Value})
 $newNames=@([regex]::Matches($freshIdentity,'shared_resource=(\S+)')|ForEach-Object{$_.Groups[1].Value})
 Require ($oldNames.Count -eq 3 -and $newNames.Count -eq 3 -and @($newNames|Where-Object{$_ -in $oldNames}).Count -eq 0) 'Fresh process reused crashed resource names'
 Health 'after-recovery'|Out-Null
 $from=([datetime]$state.StartedUtc).ToLocalTime();$failures=@()
 foreach($filter in @(@{LogName='Application';StartTime=$from;Id=1000,1001},@{LogName='System';StartTime=$from;Id=10110,10111,41,1001,4101})){
  try{$found=@(Get-WinEvent -FilterHashtable $filter -ErrorAction Stop)}catch{if($_.FullyQualifiedErrorId -notlike 'NoMatchingEventsFound*'){throw};$found=@()}
  if($filter.LogName -eq 'Application'){$found=@($found|Where-Object Message -match 'WUDFHost|SweetDisplayDriver|SweetDisplayGpuPattern')};$failures+=$found
 }
 ConvertTo-Json -InputObject @($failures|Select-Object TimeCreated,Id,ProviderName,Message) -Depth 4|Set-Content (Join-Path $dir 'failure-events.json') -Encoding utf8
 $state.FailureEventCount=$failures.Count;Require (!$failures.Count) 'Relevant driver/system failure during recovery'
 $state.Outcome='PASS_RECOVERY_REVIEW_REQUIRED'
}catch{$state.Outcome='FAIL';$state.Error=$_.Exception.Message}
finally{
 if($crashHost -and !$crashHost.HasExited){$crashHost.Kill();$state.UnplannedCrashCleanup=$true}
 if($pattern -and !$pattern.HasExited){try{CloseCrashPattern}catch{$state.Outcome='FAIL';$state.CleanupError=$_.Exception.Message;if(!$pattern.HasExited){$pattern.Kill();$state.PatternForcedStop=$true}}}
 $state.CompletedUtc=[datetime]::UtcNow.ToString('o');Save
}
if($state.Outcome -ne 'PASS_RECOVERY_REVIEW_REQUIRED'){throw $state.Error}
Write-Output 'PASS recovery; final independent health/event/resource review still required'
