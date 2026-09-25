# Test-controller completion policy, not driver/transport policy.
function Wait-TransportReceiverDrain {
 param([Parameter(Mandatory=$true)]$HostProcess,
       [Parameter(Mandatory=$true)][uint32]$ReceiverExitCode,
       [Parameter(Mandatory=$true)][string]$ReceiverDirectory,
       [ValidateRange(100,5000)][int]$GraceMilliseconds=5000)
 if($ReceiverExitCode -ne 0){throw ('Receiver exited with code '+$ReceiverExitCode)}
 $resultPath=Join-Path $ReceiverDirectory 'receiver-result.json'
 if(!(Test-Path -LiteralPath $resultPath)){throw 'Receiver exited without final drain evidence'}
 $r=[IO.File]::ReadAllText($resultPath)|ConvertFrom-Json
 if(!$r.drained -or $r.frames -lt 1 -or $r.protocol_errors -ne 0){throw 'Receiver exited without successful protocol drain'}
 $tail=@(Get-Content -LiteralPath (Join-Path $ReceiverDirectory 'protocol-messages.csv') -Tail 2)
 if($tail.Count -ne 2){throw 'Missing receiver DRAIN transcript'}
 $rows=@($tail|ConvertFrom-Csv -Header direction,type,session,sequence,timestamp_ns,payload,ack_count,ack_sequence,ack_frame,ack_bytes)
 if($rows[0].direction -ne 'RX' -or $rows[1].direction -ne 'TX' -or $rows[0].type -ne '5' -or $rows[1].type -ne '5' -or $rows[0].payload -ne '8' -or $rows[1].payload -ne '8' -or $rows[0].session -ne $rows[1].session){throw 'Receiver exit lacks DRAIN/DRAIN_ACK transcript'}
 # Device sends DRAIN_ACK and exits before Host finishes its own final reporting.
 # This bounded grace never turns a missing drain, Host error or hang into a pass.
 if(!$HostProcess.WaitForExit($GraceMilliseconds)){throw 'Host did not finish within receiver-drain grace'}
 return [pscustomobject]@{ReceiverDrained=$true;HostExited=$true;GraceMilliseconds=$GraceMilliseconds;Session=$rows[0].session;HostExitCodeStillRequiresCheck=$true}
}
