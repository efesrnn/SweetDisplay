# Mutate ignored fixture copies, never original live evidence.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[string]$DecodeSubdirectory='')
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$source=(Resolve-Path $EvidenceDirectory).Path
# The verifier intentionally requires a phase3b path component.
$root=Join-Path $repo ('out/phase3b/verifier-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $root|Out-Null
$verify=Join-Path $PSScriptRoot 'Verify-HardwareEncoding.ps1'
function EditJson($path,[scriptblock]$edit){$v=Get-Content $path -Raw|ConvertFrom-Json;& $edit $v;$v|ConvertTo-Json -Depth 10|Set-Content $path -Encoding utf8}
function EditCsv($path,[scriptblock]$edit){$v=@(Import-Csv $path);& $edit $v;$v|Export-Csv $path -NoTypeInformation -Encoding utf8}
function Reject($name,$expected,[scriptblock]$edit){
 $dir=Join-Path $root $name;Copy-Item $source $dir -Recurse;& $edit $dir
 $message=$null;try{& $verify -EvidenceDirectory $dir -MinimumSeconds 20 -DecodeSubdirectory $DecodeSubdirectory|Out-Null}catch{$message=$_.Exception.Message}
 if(!$message -or $message -notlike ('*'+$expected+'*')){throw ('Fixture failed: '+$name+'; '+$message)}
 return @{Case=$name;Outcome='PASS_REJECTED';Message=$message}
}
$valid=Join-Path $root 'valid';Copy-Item $source $valid -Recurse
& $verify -EvidenceDirectory $valid -MinimumSeconds 20 -DecodeSubdirectory $DecodeSubdirectory|Out-Null
$results=@(@{Case='valid';Outcome='PASS'})
$results+=Reject 'software-fallback' 'Hardware/GPU/shutdown' {param($p) EditJson "$p/host/encode-result.json" {param($v)$v.software_fallback=$true}}
$results+=Reject 'lost-input' 'Exact encoder queue/drop accounting' {param($p) EditJson "$p/host/encode-result.json" {param($v)$v.rate_drops--}}
$results+=Reject 'queue-overflow' 'Exact encoder queue/drop accounting' {param($p) EditJson "$p/host/encode-result.json" {param($v)$v.queue_peak=5}}
$results+=Reject 'false-byte-total' 'Bitstream/AU byte accounting' {param($p) [IO.File]::AppendAllText("$p/host/stream.h264",'x')}
$results+=Reject 'wrong-content' 'Decoded/source content correspondence' {param($p) EditCsv (Join-Path "$p/host/$DecodeSubdirectory" 'decoded-frames.csv') {param($v)$v[0].counter=([uint32]$v[0].counter -bxor 1).ToString()}}
$results+=Reject 'wrong-pts' 'Decoded/source content correspondence' {param($p) EditCsv (Join-Path "$p/host/$DecodeSubdirectory" 'decoded-frames.csv') {param($v)$v[0].pts=([long]$v[0].pts+1).ToString()}}
$results+=Reject 'wrong-visible-bounds' 'Decoded visible aperture bounds' {param($p) EditCsv (Join-Path "$p/host/$DecodeSubdirectory" 'decoded-frames.csv') {param($v)$v[0].coded_width='799'}}
$results|ConvertTo-Json -Depth 4|Set-Content (Join-Path $root 'results.json') -Encoding utf8
'PASS '+$results.Count+' hardware encoding evidence fixtures'
