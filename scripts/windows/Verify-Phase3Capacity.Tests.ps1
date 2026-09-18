# Mutates copies of completed evidence only; never the original run or deployment.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$fixtures=Join-Path $repo ('out/phase3-verifier-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $fixtures|Out-Null
$verify=Join-Path $PSScriptRoot 'Verify-Phase3Capacity.ps1'
$source=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
$results=[Collections.Generic.List[object]]::new()
function EditJson([string]$Path,[scriptblock]$Change){$data=Get-Content -LiteralPath $Path -Raw|ConvertFrom-Json;& $Change $data;$data|ConvertTo-Json -Depth 20|Set-Content -LiteralPath $Path -Encoding utf8}
function EditFrames([string]$Path,[scriptblock]$Change){$data=@(Import-Csv -LiteralPath $Path);& $Change $data;$data|Export-Csv -LiteralPath $Path -NoTypeInformation -Encoding utf8}
function Reject([string]$Name,[string]$Expected,[scriptblock]$Change,[int]$Minimum=0){
 $dir=Join-Path $fixtures $Name;Copy-Item -LiteralPath $source -Destination $dir -Recurse
 & $Change $dir
 $failure=$null
 try {& $verify -EvidenceDirectory $dir -MinimumStreamSeconds $Minimum|Out-Null}catch{$failure=$_.Exception.Message}
 if(!$failure -or $failure -notlike ('*'+$Expected+'*')){throw "Negative test $Name failed: $failure"}
 $results.Add(@{Test=$Name;Outcome='PASS';RejectedReason=$failure})
}
$positive=Join-Path $fixtures 'positive';Copy-Item -LiteralPath $source -Destination $positive -Recurse
& $verify -EvidenceDirectory $positive|Out-Null
$results.Add(@{Test='unchanged-completed-evidence';Outcome='PASS'})
Reject 'duplicate-id' 'clocks/ID ordering' {param($d) EditFrames (Join-Path $d 'main/session-1-frames.csv') {param($a)$a[1].frame_id=$a[0].frame_id}}
Reject 'clock-regression' 'clocks/ID ordering' {param($d) EditFrames (Join-Path $d 'main/session-1-frames.csv') {param($a)$a[1].source_qpc=$a[0].source_qpc}}
Reject 'wrong-pattern' 'pattern correspondence' {param($d) EditFrames (Join-Path $d 'main/session-1-frames.csv') {param($a)$a[1].counter='4294967295'}}
Reject 'wrong-nonce' 'pattern correspondence' {param($d) EditFrames (Join-Path $d 'main/session-1-frames.csv') {param($a)$a[1].nonce='0'}}
Reject 'wrong-geometry' 'per-frame geometry' {param($d) EditFrames (Join-Path $d 'main/session-1-frames.csv') {param($a)$a[1].width='800'}}
Reject 'wrong-row-gap' 'row gap count' {param($d) EditFrames (Join-Path $d 'main/session-1-frames.csv') {param($a)$a[1].id_gap='999'}}
Reject 'unbounded-queue' 'telemetry connection/queue/epoch' {param($d) EditFrames (Join-Path $d 'main/session-1-telemetry.csv') {param($a)$a[0].ready='4'}}
Reject 'unaccounted-drop' 'source/drop/outstanding accounting' {param($d) EditJson (Join-Path $d 'main/session-1-result.json') {param($a)$a.source_frames++}}
Reject 'testsigning-enabled' 'Periodic security/boot/PnP health' {param($d) EditJson (Join-Path $d 'after-health.json') {param($a)$a.CiFlags=$a.CiFlags -bor 2}}
Reject 'too-short-for-soak' 'Insufficient actual soak frame span' {param($d)} 1800
Reject 'failed-execution' 'Execution did not pass' {param($d) EditJson (Join-Path $d 'execution.json') {param($a)$a.Outcome='FAIL'}}
Reject 'forced-pattern-shutdown' 'Pattern did not shut down cleanly' {param($d) EditJson (Join-Path $d 'execution.json') {param($a)$a|Add-Member -NotePropertyName PatternForcedStop -NotePropertyValue $true -Force}}
$results.ToArray()|ConvertTo-Json -Depth 5|Set-Content (Join-Path $fixtures 'results.json') -Encoding utf8
Write-Output ('PASS '+$results.Count+' capacity-evidence verifier acceptance/rejection checks; fixtures retained under ignored out/')
