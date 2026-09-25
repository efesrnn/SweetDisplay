#requires -Version 7.4
#requires -RunAsAdministrator
<# DEVICE PHASE 2C-T1 interactive real-phone reconnect suite.
The operator only performs the requested touches; this controller owns bounded
ADB receiver stop/start, evidence predicates, normal Host shutdown and cleanup. #>
param(
    [Parameter(Mandatory = $true)][string]$RunName,
    [ValidateRange(1024, 65535)][int]$Port = 48231,
    [ValidateRange(1, 9)][int]$StartAt = 1,
    [string]$AdbPath = (Join-Path $env:LOCALAPPDATA 'Android\Sdk\platform-tools\adb.exe')
)
$ErrorActionPreference = 'Stop'
[Threading.Thread]::CurrentThread.CurrentCulture = [Globalization.CultureInfo]::InvariantCulture
if ($RunName -notmatch '^[a-zA-Z0-9-]+$') { throw 'Invalid run name' }
if (!(Test-Path -LiteralPath $AdbPath -PathType Leaf)) { throw 'ADB executable not found' }
$repo = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$run = Join-Path $repo ('docs\evidence\private\device-phase2ct1\' + $RunName)
if (Test-Path -LiteralPath $run) { throw 'Preserve evidence: use a new run name' }
New-Item -ItemType Directory -Path $run | Out-Null
$hostDir = Join-Path $run 'host'; New-Item -ItemType Directory -Path $hostDir | Out-Null
$stopFile = Join-Path $run 'normal-host-stop.request'
$result = [ordered]@{ Outcome='RUNNING'; StartedUtc=[datetime]::UtcNow.ToString('o'); Port=$Port; StartAt=$StartAt; UsbBefore=$null; UsbAfter=$null; HostExitCode=$null; PatternExitCode=$null; Cycles=@(); Error=$null }
function Save-Result { $result | ConvertTo-Json -Depth 10 | Set-Content -LiteralPath (Join-Path $run 'controller.json') -Encoding utf8 }
function Invoke-Adb([Parameter(ValueFromRemainingArguments=$true)][string[]]$Arguments) { $text=& $AdbPath @Arguments 2>&1; if($LASTEXITCODE -ne 0){throw ('adb failed: '+($text -join ' '))}; return @($text) }
function Wait-Until([scriptblock]$Predicate,[int]$Seconds,[string]$Failure,[int]$PollMilliseconds=100) { $deadline=[datetime]::UtcNow.AddSeconds($Seconds); do { try { if(& $Predicate){return} } catch [System.IO.IOException] {} ; Start-Sleep -Milliseconds $PollMilliseconds } while([datetime]::UtcNow -lt $deadline); throw $Failure }
function Read-Touch { $path=Join-Path $hostDir 'touch-events.csv'; if(!(Test-Path -LiteralPath $path)){return @()}; return @(Import-Csv -LiteralPath $path) }
function Touch-Sessions { return @((Read-Touch)|Where-Object event -eq 'SESSION_READY') }
function Session-Events([string]$Session) { return @((Read-Touch)|Where-Object {$_.session -eq $Session -and $_.event -in 'DOWN','MOVE','UP','CANCEL'}) }
function Contact-State([object[]]$Events) { $active=[Collections.Generic.HashSet[string]]::new(); foreach($event in $Events){$id=[string]$event.contact;if($event.event -eq 'DOWN'){$null=$active.Add($id)}elseif($event.event -in 'UP','CANCEL'){$null=$active.Remove($id)}}; return $active }
function Read-Target { $path=Join-Path $run 'touch-target-events.csv'; if(!(Test-Path -LiteralPath $path)){return @()}; return @(Import-Csv -LiteralPath $path) }
function Target-State { $active=[Collections.Generic.HashSet[string]]::new();foreach($event in (Read-Target)){$id=[string]$event.pointer_id;if($event.event -eq 'DOWN'){$null=$active.Add($id)}elseif($event.event -eq 'UP'){$null=$active.Remove($id)}};return $active }
function Api-Failures { $path=Join-Path $hostDir 'touch-api.csv';if(!(Test-Path -LiteralPath $path)){return @()};return @((Import-Csv -LiteralPath $path)|Where-Object success -eq '0') }
function Get-Health([string]$Name) { $devices=@(Get-PnpDevice -Class Display,Monitor -PresentOnly|Where-Object {$_.FriendlyName -like '*SweetDisplay*' -or $_.InstanceId -match 'SweetDisplay|SWT0001'}|ForEach-Object{[pscustomobject]@{Class=$_.Class;Status=$_.Status;Problem=(Get-PnpDeviceProperty -InstanceId $_.InstanceId -KeyName DEVPKEY_Device_ProblemCode).Data}});$health=[ordered]@{Utc=[datetime]::UtcNow.ToString('o');SecureBoot=Confirm-SecureBootUEFI;Hvci=Get-ItemPropertyValue 'HKLM:\SYSTEM\CurrentControlSet\Control\CI\State' HVCIEnabled;Boot=(Get-CimInstance Win32_OperatingSystem).LastBootUpTime.ToUniversalTime().ToString('o');Devices=$devices};$health|ConvertTo-Json -Depth 5|Set-Content -LiteralPath (Join-Path $run ($Name+'-health.json')) -Encoding utf8;if(!$health.SecureBoot-or$health.Hvci-ne1-or$devices.Count-ne2-or@($devices|Where-Object{$_.Status-ne'OK'-or$_.Problem-ne0}).Count){throw 'Windows security/PnP health failed'};return $health }
function Wait-NewSession([int]$Count) { Wait-Until { (Touch-Sessions).Count -ge $Count } 25 'Fresh touch session deadline'; return (Touch-Sessions)[-1] }
function Wait-FreshTap([string]$Session) { Write-Host 'Yeni oturum hazır. Telefona bir kez kısa dokunup bırak.' -ForegroundColor Cyan; Wait-Until { (Session-Events $Session).Count -ge 2 } 60 'Fresh tap deadline';$events=Session-Events $Session;if($events[0].event-ne'DOWN'){throw 'Fresh session did not begin with DOWN'};Wait-Until { (Contact-State (Session-Events $Session)).Count -eq 0 } 10 'Fresh tap did not finish'; }
function Wait-HeldContact([string]$Session,[string]$Kind,[string]$Prompt) {
    $expected=$(if($Kind-eq'two-down'){2}else{1})
    Wait-Until { (Contact-State (Session-Events $Session)).Count -eq 0 -and (Target-State).Count -eq 0 } 10 'Previous contact state did not settle'
    for($attempt=1;$attempt-le3;$attempt++) {
        Write-Host $Prompt -ForegroundColor Yellow
        Wait-Until {
            $events=Session-Events $Session;$active=Contact-State $events
            if($Kind-eq'single-move'){return $active.Count-eq1-and@($events|Where-Object event -eq 'MOVE').Count}
            return $active.Count-eq$expected
        } 60 'Required real touch state deadline' 20
        $deadline=[datetime]::UtcNow.AddSeconds(10)
        do {
            $events=Session-Events $Session;$hostActive=Contact-State $events;$targetActive=Target-State
            $moveReady=$Kind-ne'single-move'-or@($events|Where-Object event -eq 'MOVE').Count
            if($hostActive.Count-eq$expected-and$targetActive.Count-eq$expected-and$moveReady){return}
            if($hostActive.Count-lt$expected) { break }
            Start-Sleep -Milliseconds 20
        } while([datetime]::UtcNow-lt$deadline)
        if((Contact-State (Session-Events $Session)).Count-eq$expected){throw 'Independent target did not observe active contact state while held'}
        if($attempt-lt3){Write-Host 'Temas denetleyici kesmeden kalktı. Parmaklarını tamamen bırak; aynı tur yeniden denenecek.' -ForegroundColor Cyan;Wait-Until { (Contact-State (Session-Events $Session)).Count -eq 0 -and (Target-State).Count -eq 0 } 10 'Released contact state did not settle'}
    }
    throw 'Active-contact hold ended before controlled disconnect after three attempts'
}
function Run-Disconnect([string]$Kind,[int]$Ordinal) {
    $sessionRow=(Touch-Sessions)[-1];$session=[string]$sessionRow.session;$beforeTargetUps=@((Read-Target)|Where-Object event -eq 'UP').Count
    if($Kind-eq'single-down'){$prompt="[$Ordinal/9] Telefonda TEK parmakla bas ve BASILI TUT."}
    elseif($Kind-eq'single-move'){$prompt="[$Ordinal/9] Telefonda TEK parmakla bas, hareket ettir ve BASILI TUT."}
    else{$prompt="[$Ordinal/9] Telefonda İKİ parmakla bas ve ikisini de BASILI TUT."}
    Wait-HeldContact $session $Kind $prompt
    $stopped=[datetime]::UtcNow.ToString('o');Invoke-Adb shell am force-stop com.sweetdisplay.receiver|Out-Null
    Wait-Until { (Target-State).Count -eq 0 } 10 'Independent target retained contact after receiver disconnect'
    Wait-Until { @((Read-Touch)|Where-Object {$_.session-eq$session-and$_.event-eq'RELEASE_ALL'}).Count -eq 1 } 10 'Host did not record clean release'
    if((Api-Failures).Count){throw 'InjectTouchInput failure in accepted reconnect cycle'}
    $priorSessionCount=(Touch-Sessions).Count;Invoke-Adb shell am start -n com.sweetdisplay.receiver/.MainActivity|Out-Null;$new=Wait-NewSession ($priorSessionCount+1)
    if([string]$new.session-eq$session){throw 'Reconnect reused retired session'}
    Wait-FreshTap ([string]$new.session)
    $afterTargetUps=@((Read-Target)|Where-Object event -eq 'UP').Count
    $entry=[ordered]@{Ordinal=$Ordinal;Kind=$Kind;StoppedUtc=$stopped;OldSessionReleased=$true;FreshSession=$true;FreshDownRequired=$true;TargetUpsDelta=$afterTargetUps-$beforeTargetUps};$result.Cycles+=@($entry);Save-Result
}
$pattern=$null;$hostProcess=$null;$forwardCreated=$false;$before=$null
try {
    Save-Result;$before=Get-Health 'before'
    if(@(Get-Process SweetDisplayDevice -ErrorAction SilentlyContinue).Count-ne1){throw 'Unique existing SweetDisplayDevice helper required'}
    if((Invoke-Adb get-state|Select-Object -First 1)-ne'device'){throw 'ADB device is not ready'}
    $result.UsbBefore=(Invoke-Adb shell getprop sys.usb.config|Select-Object -First 1).Trim();if($result.UsbBefore-ne'mtp,adb'){throw 'Unexpected USB composition'}
    if(!(Invoke-Adb shell pm list packages com.sweetdisplay.receiver|Select-String 'package:com.sweetdisplay.receiver')){throw 'Android receiver is not installed'}
    Invoke-Adb forward ('tcp:'+$Port) ('tcp:'+$Port)|Out-Null;$forwardCreated=$true;Invoke-Adb logcat -c|Out-Null;Invoke-Adb shell am start -n com.sweetdisplay.receiver/.MainActivity|Out-Null
    $nonce='{0:X8}'-f(Get-Random -Minimum 1 -Maximum ([int]::MaxValue));$result.Nonce=$nonce
    $pattern=Start-Process -FilePath (Join-Path $repo 'out\gpu-pattern\SweetDisplayGpuPattern.exe') -ArgumentList ('"'+$run+'" '+$nonce+' 900 0 1 touch') -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $run 'pattern-stdout.txt') -RedirectStandardError (Join-Path $run 'pattern-stderr.txt')
    Wait-Until { Test-Path -LiteralPath (Join-Path $run 'touch-target-events.csv') } 15 'Independent target readiness deadline'
    $hostArgs='--output "'+$hostDir+'" --seconds 600 --stop-file "'+$stopFile+'" --first-fail --classified --nonce '+$nonce+' --encode --encode-width 800 --encode-height 360 --encode-fps 30 --encode-bitrate 8000000 --encode-uncapped --transport-port '+$Port+' --touch-mode inject'
    $hostProcess=Start-Process -FilePath (Join-Path $repo 'out\transport-host\SweetDisplayHost.exe') -ArgumentList $hostArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $hostDir 'stdout.txt') -RedirectStandardError (Join-Path $hostDir 'stderr.txt')
    $current=Wait-NewSession 1;Write-Host 'Video ve Touch Profile 1 hazır.' -ForegroundColor Green
    $specs=@(
        [pscustomobject]@{Ordinal=1;Kind='single-down'},[pscustomobject]@{Ordinal=2;Kind='single-down'},[pscustomobject]@{Ordinal=3;Kind='single-down'},
        [pscustomobject]@{Ordinal=4;Kind='single-move'},[pscustomobject]@{Ordinal=5;Kind='single-move'},[pscustomobject]@{Ordinal=6;Kind='single-move'},
        [pscustomobject]@{Ordinal=7;Kind='two-down'},[pscustomobject]@{Ordinal=8;Kind='two-down'},[pscustomobject]@{Ordinal=9;Kind='two-down'}
    )
    $selectedSpecs=@($specs|Where-Object Ordinal -ge $StartAt)
    foreach($spec in $selectedSpecs){Run-Disconnect $spec.Kind $spec.Ordinal}
    $session=[string](Touch-Sessions)[-1].session
    Wait-HeldContact $session 'single-down' '[10/10] Normal Host kapanışı: TEK parmakla bas ve BASILI TUT.'
    'normal-stop'|Set-Content -LiteralPath $stopFile -Encoding ascii
    if(!$hostProcess.WaitForExit(60000)){throw 'Host normal shutdown deadline'};$result.HostExitCode=$hostProcess.ExitCode;if($hostProcess.ExitCode-ne0){throw ('Host failed: '+[IO.File]::ReadAllText((Join-Path $hostDir 'stderr.txt')))}
    Wait-Until { (Target-State).Count -eq 0 } 10 'Target retained contact after normal Host shutdown'
    if((Api-Failures).Count){throw 'InjectTouchInput failure in accepted suite'}
    if(!$pattern.CloseMainWindow()-or!$pattern.WaitForExit(5000)){throw 'Pattern clean shutdown failed'};$result.PatternExitCode=$pattern.ExitCode;if($pattern.ExitCode-ne0){throw 'Pattern failed'}
    $transport=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'transport-result.json')|ConvertFrom-Json;$classification=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'classification-result.json')|ConvertFrom-Json;$encode=Get-Content -Raw -LiteralPath (Join-Path $hostDir 'encode-result.json')|ConvertFrom-Json
    if($transport.protocol_errors-ne0-or$transport.pending-ne0-or$transport.queue_peak-gt3-or$transport.connections-lt($selectedSpecs.Count+1)){throw 'Transport reconnect acceptance failed'}
    if($classification.B-or$classification.C-or$classification.D-or$classification.E-or!$encode.hardware_only-or$encode.software_fallback-or!$encode.drained){throw 'Video regression acceptance failed'}
    $result.Transport=$transport;$result.Classification=$classification;$result.Encoder=$encode;$result.Outcome='PASS'
} catch { $result.Outcome='ERROR';$result.Error=$_.Exception.Message }
finally {
    if($hostProcess-and!$hostProcess.HasExited){if(!(Test-Path -LiteralPath $stopFile)){'cleanup-stop'|Set-Content -LiteralPath $stopFile -Encoding ascii};if(!$hostProcess.WaitForExit(15000)){$hostProcess.Kill();$result.HostForcedStop=$true}}
    try{Invoke-Adb -Arguments @('logcat','-d','-v','threadtime','-s','SWDPRX:I','*:S')|Set-Content -LiteralPath (Join-Path $run 'android-swdprx.log') -Encoding utf8;$result.UsbAfter=(Invoke-Adb shell getprop sys.usb.config|Select-Object -First 1).Trim();if($result.UsbAfter-ne'mtp,adb'){throw 'USB composition changed'}}catch{$result.CleanupError=$_.Exception.Message;$result.Outcome='ERROR'}
    if($forwardCreated){try{Invoke-Adb forward --remove ('tcp:'+$Port)|Out-Null}catch{$result.ForwardCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}}
    if($pattern-and!$pattern.HasExited){$pattern.CloseMainWindow()|Out-Null;if(!$pattern.WaitForExit(5000)){$result.PatternCleanupIncomplete=$true;$result.Outcome='ERROR'}}
    if($pattern-and$pattern.HasExited){$result.PatternExitCode=$pattern.ExitCode}
    try{$after=Get-Health 'after';if($before-and($before.Boot-ne$after.Boot-or$before.Hvci-ne$after.Hvci)){throw 'Windows boot/security changed'}}catch{$result.HealthCleanupError=$_.Exception.Message;$result.Outcome='ERROR'}
    $result.CompletedUtc=[datetime]::UtcNow.ToString('o');Save-Result
}
if($result.Outcome-ne'PASS'){throw ($result.Outcome+': '+$result.Error+' '+$result.CleanupError+' '+$result.HealthCleanupError)}
Write-Output ('PASS: '+$RunName)
