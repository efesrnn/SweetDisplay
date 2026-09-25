#requires -Version 7.4
#requires -RunAsAdministrator
[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$FastbootPath,
    [Parameter(Mandatory=$true)][string]$AdbPath,
    [Parameter(Mandatory=$true)][string]$ImagePath,
    [Parameter(Mandatory=$true)][string]$ExpectedImageSha256,
    [Parameter(Mandatory=$true)][string]$ProbePath,
    [Parameter(Mandatory=$true)][string]$RunName,
    [switch]$OwnerPresent,
    [switch]$PhoneAccessible,
    [switch]$CableStable,
    [switch]$BatterySafe,
    [switch]$Execute
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if (!$Execute -or !$OwnerPresent -or !$PhoneAccessible -or !$CableStable -or !$BatterySafe) {
    throw 'Execution requires every physical safety confirmation'
}
if ($RunName -notmatch '^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$') { throw 'Invalid run name' }
foreach ($path in @($FastbootPath,$AdbPath,$ImagePath,$ProbePath)) {
    if (!(Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file missing: $path" }
}
$expected=$ExpectedImageSha256.ToLowerInvariant()
$actual=(Get-FileHash -LiteralPath $ImagePath -Algorithm SHA256).Hash.ToLowerInvariant()
if ($expected -notmatch '^[0-9a-f]{64}$' -or $actual -ne $expected) { throw 'Candidate SHA-256 gate failed' }

$repo=(Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$evidenceRoot=Join-Path $repo 'docs\evidence\private\final-usb\live'
$run=Join-Path $evidenceRoot $RunName
if(Test-Path -LiteralPath $run){throw 'Preserve evidence: use a new run name'}
New-Item -ItemType Directory -Path $run -Force|Out-Null

$hostIp='10.77.77.1';$phoneIp='10.77.77.2';$prefix=30;$subnet='10.77.77.0/30'
$allowedNcmRoutes=@($subnet,"$hostIp/32",'10.77.77.3/32','224.0.0.0/4','255.255.255.255/32')
$interfaceIndex=$null;$addressAdded=$false;$bootSent=$false;$outcome='ERROR';$errorText=$null
$cleanup=[Collections.Generic.List[string]]::new()
function Save-Json([string]$Name,[object]$Value){$Value|ConvertTo-Json -Depth 20|Set-Content -LiteralPath (Join-Path $run $Name) -Encoding utf8}
function Save-Text([string]$Name,[AllowNull()][string]$Value){if($null-eq$Value){$Value=''};Set-Content -LiteralPath (Join-Path $run $Name) -Value $Value -Encoding utf8}
function Canon-Defaults {(@(Get-NetRoute -AddressFamily IPv4 -DestinationPrefix '0.0.0.0/0' -ErrorAction SilentlyContinue|Sort-Object InterfaceIndex,NextHop|ForEach-Object{"$($_.InterfaceIndex)|$($_.NextHop)|$($_.RouteMetric)"})-join"`n")}
function Canon-Dns {(@(Get-DnsClientServerAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue|Sort-Object InterfaceIndex|ForEach-Object{"$($_.InterfaceIndex)|$($_.ServerAddresses-join',')"})-join"`n")}
function Canon-Profiles {(@(Get-NetConnectionProfile -ErrorAction SilentlyContinue|Sort-Object InterfaceIndex|ForEach-Object{"$($_.InterfaceIndex)|$($_.NetworkCategory)|$($_.IPv4Connectivity)|$($_.IPv6Connectivity)"})-join"`n")}
function Driver-Hash {$t=(@(& "$env:SystemRoot\System32\pnputil.exe" /enum-drivers 2>&1)|ForEach-Object{[string]$_})-join"`r`n";$sha=[Security.Cryptography.SHA256]::Create();try{([BitConverter]::ToString($sha.ComputeHash([Text.Encoding]::UTF8.GetBytes($t)))).Replace('-','')}finally{$sha.Dispose()}}
function Get-NcmAdapter {@(Get-CimInstance Win32_NetworkAdapter -ErrorAction SilentlyContinue|Where-Object{$_.ServiceName-eq'UsbNcm'-and$null-ne$_.InterfaceIndex}|Sort-Object InterfaceIndex|Select-Object -First 1)}
function Get-NcmHealth {@(Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue|Where-Object{$_.Present-and$_.Service-eq'UsbNcm'}|Select-Object Name,Service,Status,ConfigManagerErrorCode,Manufacturer)}
function Invoke-Adb([string[]]$Arguments){$o=@(&$AdbPath @Arguments 2>&1);[pscustomobject]@{ExitCode=$LASTEXITCODE;Text=(($o|ForEach-Object{[string]$_})-join"`n").Trim()}}
function Wait-Stock([int]$Seconds){
    $end=(Get-Date).AddSeconds($Seconds)
    do{
        $s=Invoke-Adb @('get-state')
        if($s.ExitCode-eq0-and$s.Text-eq'device'){
            $b=Invoke-Adb @('shell','getprop','sys.boot_completed')
            $c=Invoke-Adb @('shell','getprop','sys.usb.config')
            $u=Invoke-Adb @('shell','getprop','sys.usb.state')
            $e=Invoke-Adb @('shell','getenforce')
            if($b.Text-eq'1'-and$c.Text-eq'mtp,adb'-and$u.Text-eq'mtp,adb'-and$e.Text-eq'Enforcing'){return $true}
        }
        Start-Sleep -Milliseconds 500
    }while((Get-Date)-lt$end)
    return $false
}
function Fastboot-Getvar([string]$Name){$o=@(&$FastbootPath getvar $Name 2>&1);[pscustomobject]@{ExitCode=$LASTEXITCODE;Text=(($o|ForEach-Object{[string]$_})-join"`n")}}

$baselineDefaults=Canon-Defaults;$baselineDns=Canon-Dns;$baselineProfiles=Canon-Profiles;$baselineDrivers=Driver-Hash
try{
    $devices=@(&$FastbootPath devices 2>&1|ForEach-Object{[string]$_}|Where-Object{$_-match'\s+fastboot\s*$'})
    Save-Text 'fastboot-devices-private.txt' ($devices-join"`n")
    if($LASTEXITCODE-ne0-or$devices.Count-ne1){throw 'Exactly one bootloader-Fastboot device is required'}
    $product=Fastboot-Getvar 'product';$unlocked=Fastboot-Getvar 'unlocked'
    Save-Json 'preflight.json' ([ordered]@{ImageSha256=$actual;ImageBytes=(Get-Item -LiteralPath $ImagePath).Length;ProductOutput=$product.Text;UnlockedOutput=$unlocked.Text;NcmPresent=@(Get-NcmHealth).Count;Defaults=$baselineDefaults;Dns=$baselineDns;Profiles=$baselineProfiles;DriverHash=$baselineDrivers})
    if($product.ExitCode-ne0-or$product.Text-notmatch'(?im)product:\s*sweet\s*$'){throw 'Fastboot product is not sweet'}
    if($unlocked.ExitCode-ne0-or$unlocked.Text-notmatch'(?im)unlocked:\s*yes\s*$'){throw 'Bootloader unlocked gate failed'}
    if(@(Get-NcmHealth).Count){throw 'UsbNcm unexpectedly active before temporary boot'}
    if(@(Get-NetRoute -AddressFamily IPv4 -ErrorAction SilentlyContinue|Where-Object{$_.DestinationPrefix-eq$subnet}).Count){throw 'Selected /30 conflicts with an existing host route'}

    $bootStart=Get-Date;$bootOutput=@(&$FastbootPath boot $ImagePath 2>&1);$bootExit=$LASTEXITCODE;$bootSent=$true;$bootEnd=Get-Date
    Save-Text 'fastboot-boot-private.txt' (($bootOutput|ForEach-Object{[string]$_})-join"`n")
    if($bootExit-ne0-or(($bootOutput|ForEach-Object{[string]$_})-join"`n")-notmatch'(?i)Booting\s+OKAY'){throw 'fastboot boot did not return Booting OKAY'}

    $samples=[Collections.Generic.List[object]]::new();$adapter=$null;$end=(Get-Date).AddSeconds(60)
    do{$health=@(Get-NcmHealth);$adapters=@(Get-NcmAdapter);$samples.Add([pscustomobject]@{Timestamp=(Get-Date).ToString('o');Health=$health;Adapters=@($adapters|Select-Object Name,ServiceName,NetEnabled,NetConnectionStatus,InterfaceIndex,Speed)});if(@($health|Where-Object { $_.ConfigManagerErrorCode -eq 0 }).Count-eq1-and$adapters.Count-eq1){$adapter=$adapters[0];break};Start-Sleep -Milliseconds 250}while((Get-Date)-lt$end)
    Save-Json 'ncm-enumeration.json' @($samples)
    if($null-eq$adapter){throw 'Healthy inbox UsbNcm enumeration did not appear'}
    $interfaceIndex=[int]$adapter.InterfaceIndex
    New-NetIPAddress -InterfaceIndex $interfaceIndex -IPAddress $hostIp -PrefixLength $prefix -PolicyStore ActiveStore -AddressFamily IPv4 -ErrorAction Stop|Out-Null
    $addressAdded=$true
    $address=@();$allNcmAddresses=@();$routes=@();$addressDeadline=(Get-Date).AddSeconds(10)
    do{
        $allNcmAddresses=@(Get-NetIPAddress -InterfaceIndex $interfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue)
        $address=@($allNcmAddresses|Where-Object { $_.IPAddress -eq $hostIp })
        $routes=@(Get-NetRoute -InterfaceIndex $interfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue|Select-Object DestinationPrefix,NextHop,RouteMetric,PolicyStore)
        if($address.Count-eq1-and@($routes|Where-Object { $_.DestinationPrefix -eq $subnet }).Count-ge1){break}
        Start-Sleep -Milliseconds 250
    }while((Get-Date)-lt$addressDeadline)
    $dns=@(Get-DnsClientServerAddress -InterfaceIndex $interfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue|Select-Object -ExpandProperty ServerAddresses)
    if($address.Count-ne1-or@($routes|Where-Object { $_.DestinationPrefix -eq $subnet }).Count-lt1){throw 'Ephemeral Windows /30 configuration failed'}
    if(@($allNcmAddresses|Where-Object { $_.IPAddress -ne $hostIp }).Count){throw 'Unexpected additional IPv4 address on NCM'}
    if(@($routes|Where-Object { $_.DestinationPrefix -notin $allowedNcmRoutes }).Count){throw 'Unexpected non-project route on NCM'}
    if($routes.DestinationPrefix-contains'0.0.0.0/0'-or@($dns|Where-Object{$_}).Count){throw 'Unexpected gateway/default-route/DNS state on NCM'}
    if((Canon-Defaults)-ne$baselineDefaults){throw 'Existing Windows default route changed'}
    Save-Json 'ephemeral-addressing.json' ([ordered]@{Host=$hostIp;Phone=$phoneIp;Prefix=$prefix;Routes=$routes;Dns=$dns;PolicyStore='ActiveStore'})

    $probeJson=Join-Path $run 'protocol-probe.json';$probeOutput=@(&$ProbePath $phoneIp $hostIp $probeJson 2>&1);$probeExit=$LASTEXITCODE
    Save-Text 'protocol-probe-output.txt' (($probeOutput|ForEach-Object{[string]$_})-join"`n")
    if($probeExit-ne0-or!(Test-Path -LiteralPath $probeJson)){throw 'Direct NCM TCP SWDP probe failed'}
    $probe=Get-Content -Raw -LiteralPath $probeJson|ConvertFrom-Json
    if($probe.outcome-ne'PASS'-or$probe.reconnects-ne1-or!$probe.payload_integrity-or!$probe.sequence_integrity-or$probe.clean_drains-ne2){throw 'SWDP probe acceptance failed'}
    $outcome='PROTOCOL_PASS_PENDING_STOCK_RETURN'
}
catch{$errorText=($_|Out-String).Trim();Save-Text 'controller-error-private.txt' $errorText;if($outcome-eq'ERROR'){$outcome=if($bootSent){'FAILED'}else{'BLOCKED'}}}
finally{
    if($addressAdded-and$null-ne$interfaceIndex){try{Get-NetIPAddress -InterfaceIndex $interfaceIndex -AddressFamily IPv4 -ErrorAction SilentlyContinue|Where-Object { $_.IPAddress -eq $hostIp }|Remove-NetIPAddress -Confirm:$false -ErrorAction Stop}catch{$cleanup.Add('Ephemeral Windows address removal failed')}}
    $stockReturned=if($bootSent){Wait-Stock 240}else{$false}
    $phone=[ordered]@{Adb='unavailable';BootCompleted=$null;Config=$null;State=$null;Build=$null;Enforcing=$null}
    if($stockReturned){$phone.Adb='device';$phone.BootCompleted=(Invoke-Adb @('shell','getprop','sys.boot_completed')).Text;$phone.Config=(Invoke-Adb @('shell','getprop','sys.usb.config')).Text;$phone.State=(Invoke-Adb @('shell','getprop','sys.usb.state')).Text;$phone.Build=(Invoke-Adb @('shell','getprop','ro.build.version.incremental')).Text;$phone.Enforcing=(Invoke-Adb @('shell','getenforce')).Text}
    $tempIp=@(Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue|Where-Object { $_.IPAddress -eq $hostIp }).Count
    $tempRoute=@(Get-NetRoute -AddressFamily IPv4 -ErrorAction SilentlyContinue|Where-Object { $_.DestinationPrefix -eq $subnet }).Count
    $finalNcm=@(Get-NcmHealth);$finalHealth=@(Get-CimInstance Win32_PnPEntity -ErrorAction SilentlyContinue|Where-Object{$_.Present-and$_.Service-in@('WinUSB','WUDFWpdMtp')}|Select-Object Name,Service,Status,ConfigManagerErrorCode)
    if($bootSent-and(!$stockReturned-or$phone.Config-ne'mtp,adb'-or$phone.State-ne'mtp,adb'-or$phone.Enforcing-ne'Enforcing')){$cleanup.Add('Healthy enforcing stock mtp,adb return was not established')}
    if($tempIp-or$tempRoute-or$finalNcm.Count){$cleanup.Add('Temporary Windows NCM state remains')}
    if((Canon-Defaults)-ne$baselineDefaults-or(Canon-Dns)-ne$baselineDns){$cleanup.Add('Default-route or DNS state changed')}
    if((Driver-Hash)-ne$baselineDrivers){$cleanup.Add('Windows driver inventory changed')}
    if($cleanup.Count){$outcome='CLEANUP/ROLLBACK FAILURE'}elseif($outcome-eq'PROTOCOL_PASS_PENDING_STOCK_RETURN'){$outcome='VERIFIED'}
    $summary=[ordered]@{RunName=$RunName;Outcome=$outcome;Error=$errorText;ImageSha256=$actual;FastbootBootSent=$bootSent;FinalPhone=$phone;FinalWindows=[ordered]@{NcmActive=$finalNcm.Count;TemporaryIpCount=$tempIp;TemporaryRouteCount=$tempRoute;Health=$finalHealth;DefaultsUnchanged=(Canon-Defaults)-eq$baselineDefaults;DnsUnchanged=(Canon-Dns)-eq$baselineDns;ProfilesUnchanged=(Canon-Profiles)-eq$baselineProfiles;DriverInventoryUnchanged=(Driver-Hash)-eq$baselineDrivers};CleanupErrors=@($cleanup);Safety=[ordered]@{FlashEraseFormatPartitionWrites=0;RootSuAdbRoot=0;SelinuxWeakening=0;PersistentNetwork=0;GatewayDnsDhcpNatBridgeIcs=0;FirewallOrProfileChanges=0;AdbTunnel=0;VideoHidFunctionFsCamera=0}}
    Save-Json 'summary.json' $summary
    Write-Output "FINAL_USB_OUTCOME=$outcome";Write-Output "STOCK_RETURN=$stockReturned";Write-Output "CLEANUP_ERRORS=$($cleanup.Count)"
}
if($outcome-eq'VERIFIED'){exit 0};if($outcome-eq'BLOCKED'){exit 5};if($outcome-eq'CLEANUP/ROLLBACK FAILURE'){exit 6};exit 4
