# Mutate ignored copies only; historical evidence and deployment are untouched.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$source=(Resolve-Path $EvidenceDirectory).Path
$root=Join-Path $repo ('out/classified-verifier-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $root|Out-Null
$verify=Join-Path $PSScriptRoot 'Verify-ClassifiedEvidence.ps1';$results=@()
function EditJson($p,[scriptblock]$edit){$v=Get-Content $p -Raw|ConvertFrom-Json;& $edit $v;$v|ConvertTo-Json -Depth 12|Set-Content $p -Encoding utf8}
function EditCsv($p,[scriptblock]$edit){$v=@(Import-Csv $p);& $edit $v;$v|Export-Csv $p -NoTypeInformation -Encoding utf8}
function Reject($name,$expected,[scriptblock]$edit){
 $d=Join-Path $root $name;Copy-Item $source $d -Recurse;& $edit $d
 $message=$null;try{& $verify -EvidenceDirectory $d|Out-Null}catch{$message=$_.Exception.Message}
 if(!$message -or $message -notlike ('*'+$expected+'*')){throw ('Negative fixture failed: '+$name+'; '+$message)}
 return @{Test=$name;Outcome='PASS';Rejection=$message}
}
& $verify -EvidenceDirectory $source|Out-Null
$results+=@{Test='valid-B-content';Outcome='PASS'}
$results+=Reject 'integrity-count' 'Classification totals' {param($d) EditJson (Join-Path $d 'host/classification-result.json') {param($v)$v.D=1}}
$results+=Reject 'unknown-count' 'Classification totals' {param($d) EditJson (Join-Path $d 'host/classification-result.json') {param($v)$v.E=1}}
$results+=Reject 'drop-mismatch' 'Exact drop accounting' {param($d) EditJson (Join-Path $d 'host/session-1-result.json') {param($v)$v.source_frames++}}
$results+=Reject 'invalid-slot' 'Geometry/slot/generation' {param($d) EditCsv (Join-Path $d 'host/session-1-frames.csv') {param($v)$v[0].slot='3'}}
$results+=Reject 'duplicate-id' 'Metadata/QPC ordering' {param($d) EditCsv (Join-Path $d 'host/session-1-frames.csv') {param($v)$v[1].frame_id=$v[0].frame_id}}
$results+=Reject 'reference-mismatch' 'Independent desktop corroboration' {param($d) EditCsv (Join-Path $d 'host/classifications.csv') {param($v)($v|Where-Object class -eq '2'|Select-Object -First 1).reference_hash2='0'}}
$results+=Reject 'reference-unavailable' 'Independent desktop corroboration' {param($d) EditCsv (Join-Path $d 'host/classifications.csv') {param($v)($v|Where-Object class -eq '2'|Select-Object -First 1).reference_error='5'}}
$results+=Reject 'association-mismatch' 'Classification/frame association' {param($d) EditCsv (Join-Path $d 'host/classifications.csv') {param($v)$v[1].frame_id=$v[0].frame_id}}
$results+=Reject 'forced-stop' 'Classified execution/clean shutdown' {param($d) EditJson (Join-Path $d 'execution.json') {param($v)$v|Add-Member PatternForcedStop $true -Force}}
$results+=Reject 'security-change' 'Security/boot state' {param($d) EditJson (Join-Path $d 'after-health.json') {param($v)$v.CiFlags=$v.CiFlags -bor 2}}
$results|ConvertTo-Json -Depth 4|Set-Content (Join-Path $root 'results.json') -Encoding utf8
'PASS '+$results.Count+' classified-evidence acceptance/rejection checks'
