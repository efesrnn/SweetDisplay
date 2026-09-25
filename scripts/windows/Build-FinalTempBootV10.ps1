param(
    [Parameter(Mandatory=$true)][string]$Python
)

$ErrorActionPreference = 'Stop'
$builder = Join-Path $PSScriptRoot 'Build-FinalTempBootV6.ps1'
& $builder -Python $Python -Revision v10
if ($LASTEXITCODE -ne 0) { throw "V10 temporary boot build failed: $LASTEXITCODE" }
