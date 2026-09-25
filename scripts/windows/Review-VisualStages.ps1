# Read-only diagnostic analysis; writes a separate, previously absent report.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[string]$OracleCsv,[string]$ReportDirectory)
$ErrorActionPreference='Stop'
. (Join-Path $PSScriptRoot Read-VisualSamples.ps1)
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
if($dir -notmatch '[\\/]docs[\\/]evidence[\\/]private[\\/]phase3d[\\/]'){throw 'Private PHASE3D diagnostic evidence required'}
$reportDir=if($ReportDirectory){(Resolve-Path -LiteralPath $ReportDirectory).Path}else{$dir}
if($reportDir -notmatch '[\\/]docs[\\/]evidence[\\/]private[\\/]phase3d[\\/]'){throw 'Private PHASE3D report required'}
$target=Join-Path $reportDir stage-review.json
if(Test-Path $target){throw 'Preserve existing stage review'}
function Require($value,$why){if(!$value){throw $why}}
function Stats($values){
 $a=@($values|Sort-Object);if(!$a.Count){return $null}
 return @{Count=$a.Count;P50=$a[[math]::Floor(($a.Count-1)*.50)];P95=$a[[math]::Floor(($a.Count-1)*.95)];P99=$a[[math]::Floor(($a.Count-1)*.99)];Max=$a[-1]}
}
$ui=@(Import-Csv (Join-Path $dir timing-ui.csv));$network=@(Import-Csv (Join-Path $dir timing-network.csv))
$durations=[Collections.Generic.List[object]]::new()
foreach($stream in @(@{Name='ui';Rows=$ui},@{Name='network';Rows=$network})){
 Require (@($stream.Rows|Where-Object event -eq 'trace_lost').Count -eq 1) 'Missing trace completion marker'
 $starts=@{};$last=[decimal]0
 foreach($r in $stream.Rows){
  Require ([decimal]$r.ns -ge $last) 'Trace timestamp order';$last=[decimal]$r.ns
  if($r.event -eq 'trace_lost'){Require ($r.value -eq '0') 'Incomplete bounded trace'}
  if($r.event -match '^(.*)_begin$'){$starts[$Matches[1]]=$r}
  elseif($r.event -match '^(.*)_end$' -and $starts.ContainsKey($Matches[1])){
   $stage=$Matches[1];$b=$starts[$stage];$starts.Remove($stage)
   $durations.Add([pscustomobject]@{Thread=$stream.Name;Stage=$stage;StartNs=$b.ns;EndNs=$r.ns;Ms=([double]([decimal]$r.ns-[decimal]$b.ns)/1e6);Frame=$b.frame_id;Session=$b.session;Generation=$b.generation;BeginValue=$b.value;EndValue=$r.value})
  }
 }
}
$intervals=@{};foreach($g in $durations|Group-Object Stage){$intervals[$g.Name]=Stats @($g.Group|ForEach-Object Ms)}
$push=@{};$submit=@{};$pop=@{};$decode=@{};$queueWait=[Collections.Generic.List[double]]::new();$decodeWait=[Collections.Generic.List[double]]::new();$renderWait=[Collections.Generic.List[double]]::new()
foreach($r in $network){if($r.event -eq 'queue_admit'){$push[$r.session+':'+$r.frame_id]=$r}}
foreach($r in $ui){$key=$r.session+':'+$r.frame_id
 if($r.event -eq 'queue_pop'){Require ($push.ContainsKey($key)) 'Pop without admitted frame';$queueWait.Add([double]([decimal]$r.ns-[decimal]$push[$key].ns)/1e6);$pop[$key]=$r}
 if($r.event -eq 'submission_accepted'){$submit[$key]=$r}
 if($r.event -eq 'decoded_surface'){Require ($submit.ContainsKey($key)) 'Decoded frame without accepted submission';$decodeWait.Add([double]([decimal]$r.ns-[decimal]$submit[$key].ns)/1e6);$decode[$key]=$r}
 if($r.event -eq 'render_begin'){Require ($decode.ContainsKey($key)) 'Render without decoded association';$renderWait.Add([double]([decimal]$r.ns-[decimal]$decode[$key].ns)/1e6)}
}
$overflows=@();$pending=$null
foreach($r in $network){
 if($r.event -eq 'queue_overflow'){
  $active=@($durations|Where-Object {$_.Thread -eq 'ui' -and [decimal]$_.StartNs -le [decimal]$r.ns -and [decimal]$_.EndNs -ge [decimal]$r.ns})
  $pending=@{Frame=$r.frame_id;Session=$r.session;Ns=$r.ns;OverlappingUiStages=$active;DependentSkips=0;RecoveryWaitMs=$null};$overflows+=,$pending
 }elseif($r.event -eq 'resync_skip' -and $pending){$pending.DependentSkips++}
 elseif($r.event -eq 'idr_recovery' -and $pending){$pending.RecoveryFrame=$r.frame_id;$pending.RecoveryWaitMs=[double]([decimal]$r.ns-[decimal]$pending.Ns)/1e6;$pending=$null}
}
$v=Get-Content (Join-Path $dir visual-result.json) -Raw|ConvertFrom-Json
Require ($v.queue_overflow -eq $overflows.Count) 'Overflow trace exact count'
Require ($v.encoded_queue_peak -le 3 -and $v.decoder_pending_peak -le 16 -and $v.render_queue_bound -eq 1 -and !$v.software_fallback -and !$v.decode_failures) 'Diagnostic queue and decoder bounds'
Require ($v.received -eq $v.admitted+$v.queue_overflow+$v.resync_skips -and $v.admitted -eq $v.queue_taken+$v.queue_reset_drops+$v.queue_pending -and $v.queue_taken -eq $v.submitted -and $v.submitted -eq $v.decoded+$v.decoder_reset_drops -and $v.decoded -eq $v.presented+$v.render_drops) 'Exact receive/decode/render accounting'
$oracle=@{};$repo=Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
if(!$OracleCsv){$OracleCsv=Join-Path $repo 'docs/evidence/private/phase3c/flow-content-3-normal/host/decode/decoded-frames.csv'}
foreach($r in Import-Csv -LiteralPath $OracleCsv){$oracle[$r.pts]=$r}
$rendered=@(Read-VisualSamples $dir);$matches=0
$received=@{};foreach($r in Import-Csv (Join-Path $dir received.csv)){$received[$r.session+':'+$r.frame_id]=$r}
foreach($r in $rendered){$o=$received[$r.session+':'+$r.frame_id];Require ($o -and $o.sequence -eq $r.sequence -and $o.pts -eq $r.pts -and $o.source_ns -eq $r.source_ns -and $o.receive_ns -eq $r.receive_ns) 'Decoded/rendered frame metadata association'}
foreach($r in $rendered|Where-Object {$_.presented -eq '1' -and $_.diagnostic -eq '1'}){
 $o=$oracle[$r.pts];Require ($o -and $o.nonce -eq $r.nonce -and $o.counter -eq $r.counter -and $r.ambiguous -eq '0') 'Recorded source/render correspondence';$matches++
}
$result=@{Outcome='DIAGNOSTIC_REVIEW_ONLY';LiveAcceptance=$false;RowsUi=$ui.Count;RowsNetwork=$network.Count;StagesMs=$intervals;QueueWaitMs=Stats $queueWait;SubmissionToOutputMs=Stats $decodeWait;DecodedToRenderMs=Stats $renderWait;Overflows=$overflows;LongestStages=@($durations|Sort-Object Ms -Descending|Select-Object -First 20);Counts=$v;ExactAccounting=$true;RecordedMarkerMatches=$matches;HistoricalRootCause='UNKNOWN'}
$result|ConvertTo-Json -Depth 12|Set-Content $target
$result|ConvertTo-Json -Depth 12
