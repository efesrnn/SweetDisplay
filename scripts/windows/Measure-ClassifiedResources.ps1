# Descriptive bounded-test resource trends, not an automatic leak-proof claim.
param([Parameter(Mandatory=$true)][string]$EvidenceDirectory,[double]$WarmupSeconds=120,[string]$ReportPath)
$ErrorActionPreference='Stop'
$dir=(Resolve-Path $EvidenceDirectory).Path
$rows=@(Import-Csv (Join-Path $dir 'resources.csv'))
function Median($values){$sorted=@($values|Sort-Object);if(!$sorted.Count){return $null};$mid=[int][Math]::Floor($sorted.Count/2);if($sorted.Count%2){return $sorted[$mid]};return ($sorted[$mid-1]+$sorted[$mid])/2}
$results=@()
foreach($g in $rows|Group-Object Role){
 $samples=@($g.Group|Where-Object {[double]$_.ElapsedSeconds -ge $WarmupSeconds})
 if($samples.Count -lt 8){throw ('Insufficient post-warmup resource samples: '+$g.Name)}
 $metrics=@{}
 $keys=@('PrivateBytes','Handles');if($samples[0].PSObject.Properties.Name -contains 'WorkingSetBytes'){$keys+='WorkingSetBytes'}
 foreach($key in $keys){
  $xs=@($samples|ForEach-Object {[double]$_.ElapsedSeconds});$ys=@($samples|ForEach-Object {[double]$_.$key})
  $mx=($xs|Measure-Object -Average).Average;$my=($ys|Measure-Object -Average).Average
  $numerator=0.0;$denominator=0.0
  for($i=0;$i -lt $samples.Count;$i++){$numerator+=($xs[$i]-$mx)*($ys[$i]-$my);$denominator+=($xs[$i]-$mx)*($xs[$i]-$mx)}
  $quarters=@();for($q=0;$q -lt 4;$q++){$lo=[int][Math]::Floor($q*$ys.Count/4);$hi=[int][Math]::Floor(($q+1)*$ys.Count/4)-1;$quarters+=Median $ys[$lo..$hi]}
  $metrics[$key]=@{First=$ys[0];Last=$ys[-1];Delta=$ys[-1]-$ys[0];Minimum=($ys|Measure-Object -Minimum).Minimum;Maximum=($ys|Measure-Object -Maximum).Maximum;SlopePerSecond=$numerator/$denominator;QuarterMedians=$quarters}
 }
 $span=[double]$samples[-1].ElapsedSeconds-[double]$samples[0].ElapsedSeconds
 $results+=@{Role=$g.Name;Samples=$samples.Count;FirstElapsedSeconds=[double]$samples[0].ElapsedSeconds;LastElapsedSeconds=[double]$samples[-1].ElapsedSeconds;Metrics=$metrics;CpuPercentOfOneLogicalProcessor=100*([double]$samples[-1].CpuSeconds-[double]$samples[0].CpuSeconds)/$span}
}
$result=@{WarmupSeconds=$WarmupSeconds;Profiles=$results;Interpretation='Review ranges, deltas, slope and quarter medians together; these observations are not proof against arbitrarily small or future leaks.'}
if(!$ReportPath){$ReportPath=Join-Path $dir 'resource-trends.json'}
$result|ConvertTo-Json -Depth 9|Set-Content $ReportPath -Encoding utf8
$result|ConvertTo-Json -Depth 9
