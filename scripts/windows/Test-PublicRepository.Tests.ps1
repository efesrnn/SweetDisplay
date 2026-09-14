#requires -Version 7.0
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$checker = Join-Path $PSScriptRoot 'Test-PublicRepository.ps1'
$testRoot = Join-Path $root ('out/hygiene-tests/' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $testRoot -Force | Out-Null
$passed = 0
function New-Fixture([string]$Name) {
    $path = Join-Path $testRoot $Name
    New-Item -ItemType Directory -Path $path -Force | Out-Null
    & git init --quiet $path
    if ($LASTEXITCODE) { throw 'Fixture init failed.' }
    return $path
}
function Expect([string]$Path, [int]$Exit, [string]$Label) {
    $output = & pwsh -NoProfile -File $checker -Repository $Path
    if ($LASTEXITCODE -ne $Exit) { throw "Unexpected checker result for $Label" }
    if ($Exit -eq 1 -and -not ($output -match '^FAIL:')) { throw 'Missing failure reason.' }
    $script:passed++
    Write-Output "PASS: $Label"
}
$clean = New-Fixture 'clean'
Set-Content (Join-Path $clean 'README.md') 'Public source description'
Expect $clean 0 'clean repository accepted'

$staged = New-Fixture 'staged'
$fake = 'ghp_' + ('A' * 36)
Set-Content (Join-Path $staged 'config.txt') $fake
& git -C $staged add config.txt
if ($LASTEXITCODE) { throw 'Fixture staging failed.' }
Set-Content (Join-Path $staged 'config.txt') 'Clean working copy'
Expect $staged 1 'secret retained only in index rejected'

$history = New-Fixture 'history'
Set-Content (Join-Path $history 'config.txt') $fake
& git -C $history add config.txt
$fixtureEmail = 'fixture' + '@' + 'example.invalid'
& git -C $history -c user.name=Fixture -c "user.email=$fixtureEmail" commit --quiet -m 'Synthetic fixture'
if ($LASTEXITCODE) { throw 'Fixture commit failed.' }
Set-Content (Join-Path $history 'config.txt') 'Clean replacement'
& git -C $history add config.txt
Expect $history 1 'secret in committed history rejected'

$forced = New-Fixture 'forced'
Set-Content (Join-Path $forced '.gitignore') '*.iso'
Set-Content (Join-Path $forced 'local.iso') 'Synthetic media placeholder'
Expect $forced 0 'ignored local media excluded'
& git -C $forced add -f local.iso
Expect $forced 1 'force-added ignored media rejected'

$unicode = New-Fixture 'unicode'
[IO.File]::WriteAllText((Join-Path $unicode 'config.txt'), $fake, [Text.Encoding]::Unicode)
Expect $unicode 1 'UTF-16 secret detected'

$personal = New-Fixture 'personal'
$homeExample = 'C:' + '\Users\' + 'FixturePerson\private'
Set-Content (Join-Path $personal 'notes.txt') $homeExample
Expect $personal 1 'personal home path detected'

$binary = New-Fixture 'binary'
[IO.File]::WriteAllBytes((Join-Path $binary 'payload.dat'), [byte[]](0,255,0,1))
Expect $binary 1 'unknown binary rejected'

$large = New-Fixture 'large'
[IO.File]::WriteAllBytes((Join-Path $large 'large.txt'), [byte[]]::new(1MB + 1))
Expect $large 1 'large generated payload rejected'
Write-Output "PASS: $passed publication-check scenarios. Synthetic fixtures retained under ignored out/hygiene-tests."

