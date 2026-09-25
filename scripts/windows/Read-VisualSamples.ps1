# Join deferred 64-pixel diagnostics by immutable frame/resource identity.
# The original ledger is never rewritten. Missing/misassociated samples fail.
function Read-VisualSamples([string]$Directory){
 $rows=@(Import-Csv -LiteralPath (Join-Path $Directory rendered.csv))
 $samplesPath=Join-Path $Directory sparse-samples.csv
 $statePath=Join-Path $Directory sparse-result.json
 if((Test-Path -LiteralPath $samplesPath) -ne (Test-Path -LiteralPath $statePath)){throw 'Incomplete sparse diagnostic evidence'}
 if(!(Test-Path -LiteralPath $samplesPath)){return $rows}
 $state=Get-Content -LiteralPath (Join-Path $Directory sparse-result.json) -Raw|ConvertFrom-Json
 $visual=Get-Content -LiteralPath (Join-Path $Directory visual-result.json) -Raw|ConvertFrom-Json
 $samples=@(Import-Csv -LiteralPath $samplesPath)
 if($state.mode -ne 'async_64_pixel' -or $state.slot_bound -ne 1 -or $state.blocking_map -or $state.pending -ne 0 -or $state.submitted -ne $state.completed -or $state.completed -ne $samples.Count -or $visual.sparse_diagnostics -ne $samples.Count){throw 'Sparse diagnostic accounting/mode violation'}
 $index=@{};$seen=@{}
 foreach($r in $rows){$key=$r.session+':'+$r.sequence;if($index.ContainsKey($key) -or $r.diagnostic -ne '0'){throw 'Duplicate/mixed diagnostic frame'};$index[$key]=$r}
 foreach($s in $samples){
  $key=$s.session+':'+$s.sequence;$r=$index[$key]
  if(!$r -or $seen.ContainsKey($key)){throw 'Sparse frame missing or duplicated'};$seen[$key]=$true
  foreach($field in @('frame_id','source_ns','pts','receive_ns','decoded_ns','present_ns','presented','generation')){if($s.$field -ne $r.$field){throw ('Sparse association mismatch: '+$field)}}
  if([decimal]$s.copy_ns -lt [decimal]$r.decoded_ns -or [decimal]$s.copy_ns -gt [decimal]$r.present_ns -or [decimal]$s.ready_ns -lt [decimal]$r.present_ns -or [decimal]$s.ready_ns-[decimal]$s.copy_ns -gt 1e9){throw 'Sparse diagnostic timestamp/deadline violation'}
  $r.diagnostic='1';$r.nonce=$s.nonce;$r.counter=$s.counter;$r.ambiguous=$s.ambiguous
 }
 return $rows
}
