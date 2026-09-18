# Offline malformed-evidence checks only; no device or desktop interaction.
param()
$ErrorActionPreference='Stop'
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root=Join-Path $repo ('out/phase3b/decoder-tests/'+[guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory $root|Out-Null
$exe=Join-Path $repo 'out/phase3b/decoder/DecodeH264Evidence.exe'
$cases=@(
 @{Name='short-header';Bytes=[byte[]]@(1,2);Expected='invalid AU record length'},
 @{Name='zero-length';Bytes=[BitConverter]::GetBytes([uint32]0);Expected='invalid AU record length'},
 @{Name='oversized';Bytes=[BitConverter]::GetBytes([uint32]16777217);Expected='invalid AU record length'},
 @{Name='truncated-payload';Bytes=([BitConverter]::GetBytes([uint32]20)+[BitConverter]::GetBytes([long]0)+[byte[]]@(0,0,1));Expected='truncated AU'}
)
$results=@()
foreach($case in $cases){
 $dir=Join-Path $root $case.Name;New-Item -ItemType Directory $dir|Out-Null
 [IO.File]::WriteAllBytes((Join-Path $dir 'access-units.bin'),$case.Bytes)
 $output=@(& $exe $dir 800 360 2>&1);$code=$LASTEXITCODE
 $output|Out-String|Set-Content (Join-Path $dir 'decoder-output.txt') -Encoding utf8
 if($code -ne 1 -or ($output -join "`n") -notlike ('*'+$case.Expected+'*')){throw ('Unexpected malformed result: '+$case.Name+'; '+($output -join ' '))}
 $results+=@{Case=$case.Name;Outcome='PASS_REJECTED';ExitCode=$code;Expected=$case.Expected}
}
$results|ConvertTo-Json|Set-Content (Join-Path $root 'results.json') -Encoding utf8
'PASS '+$results.Count+' malformed AU cases; not a real bitstream acceptance result'
