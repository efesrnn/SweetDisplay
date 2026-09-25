param(
    [string]$Distribution = 'Ubuntu',
    [string]$Checkpolicy,
    [string]$Secilc,
    [string]$Configuration = 'Release',
    [ValidateSet(3, 4)][int]$Candidate = 4
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$privateTools = Join-Path $repo 'docs\evidence\private\final-usb\policy-tools'
if (-not $Checkpolicy) { $Checkpolicy = Join-Path $privateTools 'checkpolicy-root\usr\bin\checkpolicy' }
if (-not $Secilc) { $Secilc = Join-Path $privateTools 'secilc-root\usr\bin\secilc' }
$stock = Join-Path $repo 'docs\evidence\private\final-boot-prep1\recovery-ramdisk-files\sepolicy'
$deltaName = if($Candidate-eq4){'final_usb_allow_v4.cil'}else{'final_usb_allow.cil'}
$delta = Join-Path $repo "device\final\usb\$deltaName"
$outputDirectory = Join-Path $repo "out\final-device\$Configuration\final-usb-policy-$Candidate"
$stockCil = Join-Path $outputDirectory 'stock.cil'
$patched = Join-Path $repo "out\final-device\$Configuration\sepolicy-final-usb$Candidate"
$patchedCil = Join-Path $outputDirectory 'patched.cil'

foreach ($path in @($Checkpolicy,$Secilc,$stock,$delta)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Policy input missing: $path" }
}
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

function Convert-WslPath([string]$Path) {
    $resolved = (Resolve-Path -LiteralPath $Path).Path
    $value = @(& wsl.exe -d $Distribution -- wslpath -a $resolved 2>&1)
    if ($LASTEXITCODE -ne 0 -or $value.Count -ne 1) { throw "wslpath failed: $resolved" }
    [string]$value[0]
}

$linuxCheckpolicy=Convert-WslPath $Checkpolicy
$linuxSecilc=Convert-WslPath $Secilc
$linuxStock=Convert-WslPath $stock
$linuxDelta=Convert-WslPath $delta
$linuxOutputDirectory=Convert-WslPath $outputDirectory
$linuxPatched=(Convert-WslPath (Split-Path -Parent $patched)) + '/' + (Split-Path -Leaf $patched)

& wsl.exe -d $Distribution -- $linuxCheckpolicy -M -b -C -o "$linuxOutputDirectory/stock.cil" $linuxStock
if ($LASTEXITCODE -ne 0) { throw 'Stock binary policy to CIL conversion failed' }
& wsl.exe -d $Distribution -- $linuxSecilc -m -M true -X 0 -c 30 `
    -o $linuxPatched -f /dev/null "$linuxOutputDirectory/stock.cil" $linuxDelta
if ($LASTEXITCODE -ne 0) { throw 'FINAL-USB policy compile failed' }
& wsl.exe -d $Distribution -- $linuxCheckpolicy -M -b -C `
    -o "$linuxOutputDirectory/patched.cil" $linuxPatched
if ($LASTEXITCODE -ne 0) { throw 'Patched policy validation/disassembly failed' }

$bytes=[IO.File]::ReadAllBytes($patched)
if ($bytes.Length -lt 24 -or
    -not ($bytes[0] -eq 0x8c -and $bytes[1] -eq 0xff -and $bytes[2] -eq 0x7c -and $bytes[3] -eq 0xf9) -or
    [BitConverter]::ToUInt32($bytes,16) -ne 30) { throw 'Patched policy header/version mismatch' }
$stockLines=@(Get-Content -LiteralPath $stockCil)
$patchedLines=@(Get-Content -LiteralPath $patchedCil)
$patchedText=$patchedLines -join "`n"
$expectedRules=@(
    '(allow recovery self (tcp_socket (ioctl read write create getattr bind listen accept setopt shutdown)))',
    '(allow recovery node (tcp_socket (node_bind)))',
    '(allow recovery port (tcp_socket (name_bind)))'
)
foreach($rule in $expectedRules){
    if($patchedText -notmatch [regex]::Escape($rule)){throw "Patched policy rule missing: $rule"}
}
if($patchedText -match '\(allow recovery recovery \(udp_socket' -or
   $patchedText -match '\(allow recovery self \(udp_socket') {
    throw 'Unexpected recovery UDP permission in patched policy'
}

# Compare the compiled policies rather than trusting the source fragment alone.
# secilc can discard unused generated attributes while preserving every runtime
# rule, so those declarations/memberships are audited separately below.
$stockAllows=@($stockLines|Where-Object{$_ -match '^\(allow '}|Sort-Object -Unique)
$patchedAllows=@($patchedLines|Where-Object{$_ -match '^\(allow '}|Sort-Object -Unique)
$allowDelta=@(Compare-Object -ReferenceObject $stockAllows -DifferenceObject $patchedAllows |
    ForEach-Object{"$($_.SideIndicator) $($_.InputObject)"}|Sort-Object)
$expectedAllowDelta=@(
    '<= (allow recovery self (tcp_socket (ioctl create)))',
    '=> (allow recovery node (tcp_socket (node_bind)))',
    '=> (allow recovery port (tcp_socket (name_bind)))',
    '=> (allow recovery self (tcp_socket (ioctl read write create getattr bind listen accept setopt shutdown)))'
)|Sort-Object
if(@(Compare-Object -ReferenceObject $expectedAllowDelta -DifferenceObject $allowDelta).Count-ne0){
    throw "Unexpected compiled allow-rule delta: $($allowDelta -join '; ')"
}

$stockAllowX=@($stockLines|Where-Object{$_ -match '^\(allowx '}|Sort-Object -Unique)
$patchedAllowX=@($patchedLines|Where-Object{$_ -match '^\(allowx '}|Sort-Object -Unique)
$allowXDelta=@(Compare-Object -ReferenceObject $stockAllowX -DifferenceObject $patchedAllowX |
    ForEach-Object{"$($_.SideIndicator) $($_.InputObject)"}|Sort-Object)
$expectedAllowXDelta=@()
if($Candidate-eq4){
    $expectedAllowXDelta=@(
        '<= (allowx recovery self (ioctl tcp_socket (((range 0x8913 0x8914)))))',
        '=> (allowx recovery self (ioctl tcp_socket (((range 0x8913 0x8914) 0x8916 0x891c))))'
    )|Sort-Object
}
if($expectedAllowXDelta.Count-ne$allowXDelta.Count-or
   ($expectedAllowXDelta.Count-and@(Compare-Object -ReferenceObject $expectedAllowXDelta -DifferenceObject $allowXDelta).Count-ne0)){
    throw "Unexpected compiled allowxperm delta: $($allowXDelta -join '; ')"
}

$stockRuntime=@($stockLines|Where-Object{$_ -notmatch '^\((allowx?|typeattribute(set)?) '}|Sort-Object -Unique)
$patchedRuntime=@($patchedLines|Where-Object{$_ -notmatch '^\((allowx?|typeattribute(set)?) '}|Sort-Object -Unique)
if(@(Compare-Object -ReferenceObject $stockRuntime -DifferenceObject $patchedRuntime).Count-ne0){
    throw 'Unexpected non-allow runtime policy delta'
}

$stockAttributeSets=@($stockLines|Where-Object{$_ -match '^\(typeattributeset '}|Sort-Object -Unique)
$patchedAttributeSets=@($patchedLines|Where-Object{$_ -match '^\(typeattributeset '}|Sort-Object -Unique)
$attributeDelta=@(Compare-Object -ReferenceObject $stockAttributeSets -DifferenceObject $patchedAttributeSets)
if(@($attributeDelta|Where-Object{$_.SideIndicator-ne'<='}).Count-ne0){
    throw 'Patched policy unexpectedly added or rewrote a type-attribute membership'
}
$runtimeReferenceText=(@($stockLines+$patchedLines)|Where-Object{$_ -notmatch '^\(typeattribute(set)? '}|Sort-Object -Unique)-join"`n"
foreach($entry in $attributeDelta){
    if($entry.InputObject -notmatch '^\(typeattributeset ([^ ]+) '){throw 'Malformed type-attribute delta'}
    $attributeName=$Matches[1]
    if($runtimeReferenceText -match "(?<![A-Za-z0-9_-])$([regex]::Escape($attributeName))(?![A-Za-z0-9_-])"){
        throw "Compiler-pruned attribute remains runtime-referenced: $attributeName"
    }
}
$hash=(Get-FileHash -LiteralPath $patched -Algorithm SHA256).Hash.ToLowerInvariant()
$stockHash=(Get-FileHash -LiteralPath $stock -Algorithm SHA256).Hash.ToLowerInvariant()
Write-Output "POLICY=$patched"
Write-Output "BYTES=$($bytes.Length)"
Write-Output "SHA256=$hash"
Write-Output "STOCK_SHA256=$stockHash"
Write-Output "CANDIDATE=$Candidate"
Write-Output 'POLICY_VERSION=30'
Write-Output 'ENFORCING_DELTA_RULES=3'
Write-Output "ALLOWXPERM_COMMANDS_ADDED=$(if($Candidate-eq4){2}else{0})"
Write-Output 'UDP_PERMISSION_ADDED=0'
Write-Output "COMPILER_PRUNED_UNUSED_ATTRIBUTE_SETS=$($attributeDelta.Count)"
Write-Output 'COMPILED_ALLOW_DELTA_EXACT=PASS'
Write-Output 'COMPILED_ALLOWXPERM_DELTA_EXACT=PASS'
Write-Output 'NON_ALLOW_RUNTIME_DELTA=0'
Write-Output 'POLICY_VALIDATION=PASS'
