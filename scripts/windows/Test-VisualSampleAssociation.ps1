param([Parameter(Mandatory=$true)][ValidatePattern('^[a-zA-Z0-9-]+$')][string]$RunName,
 [Parameter(Mandatory=$true)][string]$FixtureDirectory)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot Read-VisualSamples.ps1)
$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$root=Join-Path $repo ('docs/evidence/private/phase3d/'+$RunName)
if(Test-Path $root){throw 'Preserve sample test evidence'}
New-Item -ItemType Directory $root|Out-Null
$result=@()
foreach($case in @('valid','generation','frame_id','pts','presented','early_ready','late_ready','duplicate','missing','pending','bound','blocking','missing_file')){
 $dir=Join-Path $root $case;New-Item -ItemType Directory $dir|Out-Null
 foreach($file in @('rendered.csv','sparse-samples.csv','sparse-result.json','visual-result.json')){
  if($case -eq 'missing_file' -and $file -eq 'sparse-samples.csv'){continue}
  Copy-Item -LiteralPath (Join-Path $FixtureDirectory $file) -Destination (Join-Path $dir $file)
 }
 if($case -ne 'missing_file'){
  $samples=@(Import-Csv (Join-Path $dir sparse-samples.csv));$s=$samples[0]
  $state=Get-Content (Join-Path $dir sparse-result.json) -Raw|ConvertFrom-Json
  $visual=Get-Content (Join-Path $dir visual-result.json) -Raw|ConvertFrom-Json
  switch($case){
   generation {$s.generation=[string]([long]$s.generation+1)}
   frame_id {$s.frame_id=[string]([long]$s.frame_id+1)}
   pts {$s.pts=[string]([long]$s.pts+1)}
   presented {$s.presented=if($s.presented -eq '1'){'0'}else{'1'}}
   early_ready {$s.ready_ns=[string]([decimal]$s.present_ns-1)}
   late_ready {$s.ready_ns=[string]([decimal]$s.copy_ns+1000000001)}
   duplicate {$samples+=,$s;$state.submitted++;$state.completed++;$visual.sparse_diagnostics++}
   missing {$samples=@($samples|Select-Object -Skip 1)}
   pending {$state.pending=1}
   bound {$state.slot_bound=2}
   blocking {$state.blocking_map=$true}
  }
  $samples|Export-Csv (Join-Path $dir sparse-samples.csv) -NoTypeInformation
  $state|ConvertTo-Json|Set-Content (Join-Path $dir sparse-result.json)
  $visual|ConvertTo-Json|Set-Content (Join-Path $dir visual-result.json)
 }
 $accepted=$false;$reason='';try{$merged=@(Read-VisualSamples $dir);$accepted=$true}catch{$reason=$_.Exception.Message}
 if($accepted -ne ($case -eq 'valid')){throw ('Sparse association test failed: '+$case)}
 $result+=@{Case=$case;Outcome='PASS';Accepted=$accepted;Reason=$reason}
}
$result|ConvertTo-Json|Set-Content (Join-Path $root tests.json)
'PASS '+$result.Count+' sparse association/corruption cases'
