# Diagnostic completed-prefix proof. This MUST NOT promote a failed live run.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory)
$ErrorActionPreference='Stop'
$dir=(Resolve-Path $EvidenceDirectory).Path
if($dir -notmatch '[\\/]phase3c[\\/]'){throw 'Private PHASE3C evidence required'}
$report=Join-Path $dir 'completed-prefix-verification.json'
if(Test-Path $report){throw 'Preserve existing verification'}
function Json($p){Get-Content -Raw (Join-Path $dir $p)|ConvertFrom-Json}
function Require($ok,$why){if(!$ok){throw $why}}
$execution=Json 'execution.json';$classes=Json 'host/classification-result.json'
Require ($execution.Outcome -eq 'ERROR' -and $classes.E -eq 1 -and $classes.D -eq 0) 'Expected failed/UNCLASSIFIED attempt; never relabel'
$hostHash=(Get-FileHash (Join-Path $dir 'host/access-units.bin') -Algorithm SHA256).Hash
$receiverHash=(Get-FileHash (Join-Path $dir 'receiver-1/access-units.bin') -Algorithm SHA256).Hash
Require ($hostHash -eq $receiverHash) 'Complete captured AU byte equality'
$inputs=@(Import-Csv (Join-Path $dir 'host/encode-input.csv'));$outputs=@(Import-Csv (Join-Path $dir 'host/encode-output.csv'))
$source=@(Import-Csv (Join-Path $dir 'host/session-1-frames.csv'));$received=@(Import-Csv (Join-Path $dir 'receiver-1/received.csv'))
$transport=@(Import-Csv (Join-Path $dir 'host/transport-frames.csv'))
$decHost=Json 'host/decode-prefix/decode-result.json';$decReceiver=Json 'receiver-1/decode-prefix/decode-result.json'
$decoded=@(Import-Csv (Join-Path $dir 'receiver-1/decode-prefix/decoded-frames.csv'))
Require ($outputs.Count -gt 1 -and $outputs.Count -eq $inputs.Count -and $outputs.Count -eq $received.Count -and $outputs.Count -eq $decoded.Count -and $outputs.Count -eq $classes.A) 'Completed prefix counts'
foreach($d in @($decHost,$decReceiver)){Require ($d.outcome -eq 'PASS_DECODE' -and $d.inputs -eq $outputs.Count -and $d.decoded -eq $outputs.Count -and $d.width -eq 2400 -and $d.height -eq 1080) 'Independent decode prefix'}
[long]$lastId=0;[long]$lastQpc=0;[long]$lastSequence=0;[long]$bytes=0
for($i=0;$i -lt $outputs.Count;$i++){
 $o=$outputs[$i];$x=$inputs[$i];$f=$source[$i];$r=$received[$i];$d=$decoded[$i]
 Require ($x.decision -eq 'accepted' -and $x.frame_id -eq $o.frame_id -and $o.frame_id -eq $f.frame_id -and $r.frame_id -eq $o.frame_id -and $r.source_qpc -eq $o.source_qpc -and $o.source_qpc -eq $f.source_qpc -and $r.pts -eq $o.pts -and $d.pts -eq $o.pts -and $d.nonce -eq $f.nonce -and $d.counter -eq $f.counter -and $d.ambiguous -eq '0') 'Source/encoder/wire/decode association'
 Require ([long]$r.frame_id -gt $lastId -and [long]$r.source_qpc -gt $lastQpc -and [long]$r.sequence -gt $lastSequence -and $r.bytes -eq $o.bytes -and $r.width -eq '2400' -and $r.height -eq '1080') 'Frame ordering/geometry'
 $flags=([int]$o.idr)+2*([int]$o.sps)+4*([int]$o.pps)+8*([int]$o.clean_point -ne 0)
 Require ([int]$r.flags -eq $flags -and [long]$r.receive_ns -ge [long]$r.send_ns -and [long]$r.latency_ns -eq [long]$r.receive_ns-[long]$r.send_ns) 'Flags/transport timing'
 $lastId=[long]$r.frame_id;$lastQpc=[long]$r.source_qpc;$lastSequence=[long]$r.sequence;$bytes+=[long]$o.bytes
}
Require (([int]$received[0].flags -band 7) -eq 7 -and @($received|Group-Object session).Count -eq 1) 'Initial recovery point/session'
$admitted=@($transport|Where-Object stage -eq 'input');$sent=@($transport|Where-Object stage -eq 'wire');$terminal=@($transport|Where-Object stage -eq 'terminal')
Require ($admitted.Count -eq $outputs.Count -and $sent.Count -eq $outputs.Count -and $terminal.Count -eq $outputs.Count -and @($admitted|Where-Object decision -ne 'admitted').Count -eq 0 -and @($terminal|Where-Object decision -ne 'acked').Count -eq 0) 'No hidden transport drops in completed prefix'
$wireIndex=@{};foreach($s in $sent){$wireIndex[$s.session+':'+$s.sequence]=$s}
foreach($r in $received){$w=$wireIndex[$r.session+':'+$r.sequence];Require ($w -and $w.frame_id -eq $r.frame_id -and $w.crc -eq $r.crc -and $w.send_ns -eq $r.send_ns -and $w.bytes -eq $r.bytes) 'Wire identity/CRC association'}
$depth=0;$peak=0;foreach($r in $transport){if($r.stage -eq 'input'){$depth++;$peak=[Math]::Max($peak,$depth)}elseif($r.stage -eq 'terminal'){$depth--};Require ($depth -ge 0 -and $depth -le 4) 'Bounded outstanding accounting'};Require ($depth -eq 0) 'Completed prefix pending count'
$lat=@($received|ForEach-Object {[double]$_.latency_ns/1e6}|Sort-Object)
function Percentile($q){$lat[[Math]::Ceiling($q*$lat.Count)-1]}
$prefixSeconds=([double]$received[-1].receive_ns-[double]$received[0].receive_ns)/1e9
$failure=Json 'host/first-failure.json';$snapshotSource=$failure.source_frames
$summary=@{Outcome='PASS_COMPLETED_PREFIX_ONLY';Phase3C='PARTIAL';OriginalExecutionOutcome=$execution.Outcome;OriginalError=$execution.Error;Encoded=$outputs.Count;Admitted=$admitted.Count;Sent=$sent.Count;ReceivedValidated=$received.Count;Acked=$terminal.Count;Decoded=$decoded.Count;Bytes=$bytes;AuFilesSha256=$hostHash;AllCapturedAuBytesIdentical=$true;TimestampNonceCounterMatches=$true;TransportDropsInCompletedPrefix=0;OutstandingPeak=$peak;QueuePeak=$(if($peak -eq 1){1}else{'NOT ISOLATED; bounded by outstanding peak'});SourceAtFailure=$snapshotSource;ClassA=$classes.A;ClassE=$classes.E;ClassD=$classes.D;ReceiverFrameSpanSeconds=$prefixSeconds;InterFrameReceiverFps=($received.Count-1)/$prefixSeconds;PayloadBitrateOverFrameSpan=8*$bytes/$prefixSeconds;TransportLatencyMs=@{P50=(Percentile .5);P95=(Percentile .95);P99=(Percentile .99);Maximum=$lat[-1]};Scope='Completed prefix only; requested duration, clean end-to-end drain and live receiver reconnect NOT VERIFIED. No encoder result was fabricated.'}
$summary|ConvertTo-Json -Depth 5|Set-Content $report -Encoding utf8
$summary|ConvertTo-Json -Depth 5
