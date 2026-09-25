param(
    [Parameter(Mandatory=$true)][string]$Python,
    [string]$NdkRoot = "$env:LOCALAPPDATA\Android\Sdk\ndk\28.2.13676358",
    [ValidateSet(1, 2, 3, 4)][int]$Candidate = 4
)

$ErrorActionPreference = 'Stop'
if ($Candidate -in @(3,4)) {
    & (Join-Path $PSScriptRoot 'Build-FinalUsbPolicy.ps1') -Candidate $Candidate
    if ($LASTEXITCODE -ne 0) { throw "FINAL-USB policy build failed: $LASTEXITCODE" }
}
& (Join-Path $PSScriptRoot 'Build-FinalUsbBinaries.ps1') -NdkRoot $NdkRoot -Candidate $Candidate
if ($LASTEXITCODE -ne 0) { throw "FINAL-USB binary build failed: $LASTEXITCODE" }
$revision = switch ($Candidate) { 1 { 'final-usb' } 2 { 'final-usb2' } 3 { 'final-usb3' } 4 { 'final-usb4' } }
& (Join-Path $PSScriptRoot 'Build-FinalTempBootV6.ps1') `
    -Python $Python -Revision $revision
if ($LASTEXITCODE -ne 0) { throw "FINAL-USB boot image build failed: $LASTEXITCODE" }
