# PHASE 3D validation adapter derived from the protected PHASE 3C checker; original unchanged.
# Independent closed-ledger/AU review. Never writes into historical runs.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[switch]$Synthetic,[switch]$Reconnect,[string]$ReviewDirectory)
$ErrorActionPreference='Stop'
$dir=(Resolve-Path -LiteralPath $EvidenceDirectory).Path
if($dir -notmatch '[\\/]phase3d[\\/]'){throw 'PHASE3D evidence required'}
$review=$null
if($ReviewDirectory){
 $review=(Resolve-Path -LiteralPath $ReviewDirectory).Path
 if($review -notmatch '[\\/]phase3d[\\/]' -or $review -eq $dir){throw 'Separate private PHASE3D review directory required'}
}
$report=Join-Path $(if($review){$review}else{$dir}) 'transport-verification.json'
function DecodeDirectory($name){if($review){Join-Path $review ($name+'-decode')}else{Join-Path (Join-Path $dir $name) 'decode'}}
if(Test-Path $report){throw 'Preserve existing verification'}
function Require($ok,$why){if(!$ok){throw $why}}
function Json($path){Get-Content -LiteralPath $path -Raw | ConvertFrom-Json}
if(-not ('SweetDisplayAuHash' -as [type])){Add-Type @'
using System;
public static class SweetDisplayAuHash {
 static readonly uint[] table=Make();
 static uint[] Make(){var t=new uint[256];for(uint i=0;i<256;i++){uint x=i;for(int k=0;k<8;k++)x=(x>>1)^((x&1)!=0?0xEDB88320u:0);t[i]=x;}return t;}
 public static uint Crc(byte[] b){uint c=0xFFFFFFFF;foreach(byte x in b)c=table[(c^x)&255]^(c>>8);return ~c;}
}
'@}
function Aus($path){
 $map=@{};$reader=[IO.BinaryReader]::new([IO.File]::OpenRead($path));$sha=[Security.Cryptography.SHA256]::Create();[long]$previous=-1
 try{while($reader.BaseStream.Position -lt $reader.BaseStream.Length){
  Require ($reader.BaseStream.Length-$reader.BaseStream.Position -ge 12) 'AU truncated header'
  $n=$reader.ReadUInt32();$pts=$reader.ReadUInt64();Require ($n -gt 0 -and $n -le 4194304 -and $pts -gt $previous) 'AU length/PTS'
  $bytes=$reader.ReadBytes([int]$n);Require ($bytes.Length -eq $n) 'AU truncated payload'
  $map[[string]$pts]=@{Bytes=$n;Crc=[SweetDisplayAuHash]::Crc($bytes);Hash=[BitConverter]::ToString($sha.ComputeHash($bytes))};$previous=$pts
 }}finally{$reader.Dispose();$sha.Dispose()};return $map
}
$hostDir=Join-Path $dir 'host';$t=Json (Join-Path $hostDir 'transport-result.json')
Require ($t.seen -eq $t.admitted+$t.disconnected+$t.resync_skipped+$t.queue_overflow -and $t.admitted -eq $t.acked+$t.queue_aborted+$t.unconfirmed -and $t.pending -eq 0 -and $t.queue_peak -le 3 -and $t.queue_bytes_peak -le 3*4194368 -and $t.protocol_errors -eq 0) 'Transport final exact accounting/bounds'
$ledger=@(Import-Csv (Join-Path $hostDir 'transport-frames.csv'));$input=@($ledger|Where-Object stage -eq input);$wire=@($ledger|Where-Object stage -eq wire);$terminal=@($ledger|Where-Object stage -eq terminal)
Require ($input.Count -eq $t.seen -and $wire.Count -eq $t.wire_complete -and $terminal.Count -eq $t.admitted) 'Transport ledger totals'
foreach($category in @(@('admitted','admitted'),@('disconnected','disconnected'),@('resync','resync_skipped'),@('overflow','queue_overflow'))){Require (@($input|Where-Object decision -eq $category[0]).Count -eq $t.($category[1])) ('Input category '+$category[0])}
foreach($category in @(@('acked','acked'),@('queue_aborted','queue_aborted'),@('unconfirmed','unconfirmed'))){Require (@($terminal|Where-Object decision -eq $category[0]).Count -eq $t.($category[1])) ('Terminal category '+$category[0])}
$pending=@{};$peak=0;$inputIds=@{};$wireMap=@{};$acked=@{};$unknown=@{}
foreach($r in $ledger){
 if($r.stage -eq 'input'){Require (!$inputIds.ContainsKey($r.frame_id)) 'Duplicate encoded input';$inputIds[$r.frame_id]=$r;if($r.decision -eq 'admitted'){$pending[$r.frame_id]=$r;$peak=[Math]::Max($peak,$pending.Count)}}
 elseif($r.stage -eq 'wire'){Require ($pending.ContainsKey($r.frame_id)) 'Wire without admission';$key=$r.session+':'+$r.sequence;Require (!$wireMap.ContainsKey($key)) 'Duplicate wire sequence';$wireMap[$key]=$r}
 elseif($r.stage -eq 'terminal'){Require ($pending.ContainsKey($r.frame_id)) 'Terminal without admission';$pending.Remove($r.frame_id);if($r.decision -eq 'acked'){$acked[$r.session+':'+$r.sequence]=$r}elseif($r.decision -eq 'unconfirmed'){$unknown[$r.frame_id]=$r}}
 else{throw 'Unknown transport ledger stage'}
 Require ($pending.Count -le 4) 'Outstanding bound'
}
Require ($pending.Count -eq 0) 'Outstanding final'
$hostMsgs=@(Import-Csv (Join-Path $hostDir 'protocol-messages.csv'));$allMsgs=@();$allMsgs+=@($hostMsgs|ForEach-Object{$_|Add-Member -NotePropertyName Endpoint -NotePropertyValue Host -PassThru})
$receivers=@(Get-ChildItem -LiteralPath $dir -Directory | Where-Object Name -match '^receiver-\d+$' | Sort-Object Name)
Require ($receivers.Count -eq $(if($Reconnect){2}else{1})) 'Receiver process count'
$allFrames=@();$captured=0;$decodedCount=0;$hostAus=$null;$outputMap=@{};$decodeMap=@{}
if(!$Synthetic){
 $e=Json (Join-Path $hostDir 'encode-result.json');Require ($e.outputs -eq $t.seen) 'Every encoded output explicitly accounted'
 $hostAus=Aus (Join-Path $hostDir 'access-units.bin');Require ($hostAus.Count -eq $e.outputs) 'Host AU count'
 foreach($row in Import-Csv (Join-Path $hostDir 'encode-output.csv')){$outputMap[$row.frame_id]=$row}
 foreach($row in Import-Csv (Join-Path (DecodeDirectory 'host') 'decoded-frames.csv')){$decodeMap[$row.pts]=$row}
}
$sessionFirst=@{};$globalIds=@{};$finalReceiver=$null
foreach($receiver in $receivers){
 $frames=@(Import-Csv (Join-Path $receiver.FullName 'received.csv'));Require ($frames.Count -gt 1) 'Receiver without frames';$allFrames+=$frames
 $msgs=@(Import-Csv (Join-Path $receiver.FullName 'protocol-messages.csv'));$allMsgs+=@($msgs|ForEach-Object{$_|Add-Member -NotePropertyName Endpoint -NotePropertyValue $receiver.Name -PassThru})
 $rxResult=Join-Path $receiver.FullName 'receiver-result.json'
 if(Test-Path $rxResult){$rr=Json $rxResult;Require ($rr.frames -eq $frames.Count -and $rr.protocol_errors -eq 0 -and $rr.peak_payload -le 4194368) 'Receiver result/bounds';$finalReceiver=$rr}
 elseif(!$Reconnect -or $receiver.Name -ne 'receiver-1'){throw 'Missing final receiver result'}
 $sessionId='';[uint64]$lastSeq=0;[uint64]$lastId=0;[uint64]$lastSource=0;[long]$lastPts=-1
 foreach($r in $frames){
  if($sessionId -ne $r.session){$sessionId=$r.session;$lastSeq=0;$lastId=0;$lastSource=0;$lastPts=-1;Require (([int]$r.flags -band 7) -eq 7 -and !$sessionFirst.ContainsKey($sessionId)) 'Fresh session recovery';$sessionFirst[$sessionId]=$r}
  Require ([uint64]$r.sequence -gt $lastSeq -and [uint64]$r.frame_id -gt $lastId -and [uint64]$r.source_qpc -gt $lastSource -and [long]$r.pts -gt $lastPts -and !$globalIds.ContainsKey($r.frame_id)) 'Receiver ordering/replay'
  $globalIds[$r.frame_id]=$true;$lastSeq=[uint64]$r.sequence;$lastId=[uint64]$r.frame_id;$lastSource=[uint64]$r.source_qpc;$lastPts=[long]$r.pts
  $w=$wireMap[$r.session+':'+$r.sequence];Require ($w -and $w.frame_id -eq $r.frame_id -and $w.source_qpc -eq $r.source_qpc -and $w.pts -eq $r.pts -and $w.bytes -eq $r.bytes -and $w.crc -eq $r.crc -and $w.send_ns -eq $r.send_ns) 'Sender/receiver association'
  Require ([decimal]$r.source_ns -eq [Math]::Floor([decimal]$r.source_qpc*1000000000/[decimal]$r.frequency) -and $r.width -eq 2400 -and $r.height -eq 1080 -and [uint64]$r.receive_ns -ge [uint64]$r.send_ns -and [uint64]$r.latency_ns -eq [uint64]$r.receive_ns-[uint64]$r.send_ns) 'Received time/geometry'
  if(!$Synthetic){$o=$outputMap[$r.frame_id];$a=$hostAus[$r.pts];Require ($o -and $o.pts -eq $r.pts -and $o.source_qpc -eq $r.source_qpc -and $o.bytes -eq $r.bytes -and $a.Bytes -eq $r.bytes -and $a.Crc -eq [uint32]$r.crc) 'Real encoded AU association'}
 }
 $capturePath=Join-Path $receiver.FullName 'access-units.bin'
 if(!$Synthetic -and (Test-Path $capturePath)){
  $aus=Aus $capturePath;$decodeDir=DecodeDirectory $receiver.Name;$dec=Json (Join-Path $decodeDir 'decode-result.json');$decoded=@(Import-Csv (Join-Path $decodeDir 'decoded-frames.csv'))
  Require ($aus.Count -eq $frames.Count -and $dec.outcome -eq 'PASS_DECODE' -and $dec.decoded -eq $frames.Count -and $decoded.Count -eq $frames.Count) 'Receiver independent decode count'
  for($i=0;$i -lt $frames.Count;$i++){$r=$frames[$i];$a=$aus[$r.pts];$h=$hostAus[$r.pts];$d=$decoded[$i];$hd=$decodeMap[$r.pts];Require ($a.Hash -eq $h.Hash -and $a.Bytes -eq $h.Bytes -and $a.Crc -eq [uint32]$r.crc -and $d.pts -eq $r.pts -and $d.nonce -eq $hd.nonce -and $d.counter -eq $hd.counter -and $d.ambiguous -eq 0 -and $d.width -eq 2400 -and $d.height -eq 1080) 'Byte identity/decoded source correspondence'}
  $captured+=$frames.Count;$decodedCount+=$dec.decoded
 }
}
Require ($finalReceiver.drained) 'Final receiver did not drain'
# Check every recorded message, including intervening heartbeats, in each direction.
$msgMap=@{}
foreach($group in $allMsgs|Group-Object Endpoint,session,direction){
 [uint64]$seq=0;[uint64]$stamp=0
 foreach($m in $group.Group){Require ([uint64]$m.sequence -eq $seq+1 -and [uint64]$m.timestamp_ns -ge $stamp) 'Full message sequence/time ordering';$seq=[uint64]$m.sequence;$stamp=[uint64]$m.timestamp_ns
  if($seq -eq 1){Require ($m.type -eq 1 -and $m.payload -eq 24) 'HELLO first'}elseif($seq -eq 2){Require ($m.type -eq 2 -and $m.payload -eq 32) 'CAPABILITIES second'}else{Require ($m.type -in @(3,5,6,7)) 'Negotiated message type'}
  if($m.type -eq 3){$w=$wireMap[$m.session+':'+$m.sequence];Require ($w -and [long]$m.payload -eq 64+[long]$w.bytes) 'FRAME exact payload length'}
  $key=$m.Endpoint+':'+$m.direction+':'+$m.session+':'+$m.sequence;Require (!$msgMap.ContainsKey($key)) 'Duplicate full message';$msgMap[$key]=$m
 }
}
foreach($m in $allMsgs|Where-Object direction -eq RX){
 $peers=if($m.Endpoint -eq 'Host'){@($receivers.Name)}else{@('Host')};$match=$null
 foreach($peer in $peers){$candidate=$msgMap[$peer+':TX:'+$m.session+':'+$m.sequence];if($candidate){Require (!$match) 'Reused session across receivers';$match=$candidate}}
 Require ($match -and $match.type -eq $m.type -and $match.payload -eq $m.payload -and $match.timestamp_ns -eq $m.timestamp_ns -and $match.ack_count -eq $m.ack_count -and $match.ack_sequence -eq $m.ack_sequence -and $match.ack_frame -eq $m.ack_frame -and $match.ack_bytes -eq $m.ack_bytes) 'Message sender/receiver equality'
}
foreach($group in $hostMsgs|Where-Object { $_.direction -eq 'RX' -and $_.type -eq 6 }|Group-Object session){
 [long]$count=0;[long]$bytes=0
 foreach($m in $group.Group){$a=$acked[$m.session+':'+$m.ack_sequence];Require ($a -and $a.frame_id -eq $m.ack_frame) 'Exact ACK identity';$count++;$bytes+=[long]$a.bytes;Require ([long]$m.ack_count -eq $count -and [long]$m.ack_bytes -eq $bytes) 'Exact cumulative ACK'}
}
Require (@($hostMsgs|Where-Object { $_.direction -eq 'RX' -and $_.type -eq 6 }).Count -eq $t.acked) 'ACK message totals'
foreach($a in $acked.Values){Require (@($allFrames|Where-Object { $_.session -eq $a.session -and $_.sequence -eq $a.sequence }).Count -eq 1) 'ACK without unique received FRAME'}
if($Reconnect){
 $e=Json (Join-Path $dir 'execution.json');Require ($e.ReceiverClosedAsPlanned -and $e.ReceiverRestarted -and $sessionFirst.Count -ge 2 -and $t.disconnected -gt 0 -and $t.resync_skipped -gt 0) 'Real disconnect/recovery evidence'
 $termination=@($e.Commands|Where-Object Command -eq 'Close own active visual receiver');Require ($termination.Count -eq 1 -and $termination[0].Result.HostStillRunning) 'Host remained active at receiver close'
}elseif(!$Synthetic){Require ($t.connections -eq 1 -and $t.wire_complete -eq $allFrames.Count -and $t.acked -eq $allFrames.Count -and !$t.unconfirmed -and !$t.queue_overflow -and !$t.queue_aborted) 'Normal live exact transport'}
$lat=@($allFrames|ForEach-Object {[double]$_.latency_ns/1e6}|Sort-Object)
$summary=@{Outcome='PASS_TRANSPORT';Synthetic=[bool]$Synthetic;Reconnect=[bool]$Reconnect;Transport=$t;ReceivedValidated=$allFrames.Count;CapturedByteIdentical=$captured;IndependentlyDecoded=$decodedCount;Sessions=$sessionFirst.Count;RecoveryPoints=@($sessionFirst.Values);OutstandingPeak=$peak;LatencyMs=@{P50=$lat[[Math]::Ceiling(.5*$lat.Count)-1];P95=$lat[[Math]::Ceiling(.95*$lat.Count)-1];P99=$lat[[Math]::Ceiling(.99*$lat.Count)-1];Max=$lat[-1]};Scope='Transport sequence/ACK/CRC and captured AU identity; source counter is not a transport sequence'}
if($review){$summary.Outcome='PASS_TRANSPORT_ARTIFACTS_ONLY';$summary.FullRunAccepted=$false;$summary.OriginalExecutionOutcome=(Json (Join-Path $dir 'execution.json')).Outcome;$summary.Scope+='; supplemental artifact review cannot certify process shutdown or change original execution outcome'}
$summary|ConvertTo-Json -Depth 8|Set-Content -LiteralPath $report
$summary|ConvertTo-Json -Depth 8


