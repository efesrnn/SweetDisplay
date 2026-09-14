param(
    [string]$MSBuild
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot -Parent) -Parent
$repo = Join-Path $root 'third_party/upstream/Windows-driver-samples'
$pin = '67d81f217bc01edf7a4320e4911c11065635acfa'
if (-not $MSBuild) {
    $selected = Get-Command MSBuild.exe -CommandType Application -ErrorAction SilentlyContinue
    if (-not $selected) { throw 'Initialize the approved WDK/EWDK build environment or pass -MSBuild explicitly. See docs/WDK_SETUP.md.' }
    $MSBuild = $selected.Source
}
if (-not (Test-Path -LiteralPath $MSBuild)) { throw 'MSBuild not found; see docs/BUILD_WINDOWS.md.' }
Write-Host "Selected MSBuild: $MSBuild"
$head = & git -C $repo rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $head -ne $pin) { throw 'Missing or incorrect upstream checkout; see third_party/README.md.' }
$dirty = & git -C $repo status --porcelain --untracked-files=no
if ($LASTEXITCODE -ne 0 -or $dirty) { throw 'Baseline must have unchanged tracked sources.' }
$logDirectory = Join-Path $root 'docs/evidence/private'
New-Item -ItemType Directory -Path $logDirectory -Force | Out-Null
$log = Join-Path $logDirectory ('idd-baseline-' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff') + '.log')
& $MSBuild (Join-Path $repo 'video/IndirectDisplay/IddSampleDriver.sln') /t:Build /p:Configuration=Debug /p:Platform=x64 /p:SignMode=Off /nologo /v:minimal /fl "/flp:logfile=$log;verbosity=normal"
$result = $LASTEXITCODE
Write-Host "MSBuild exit=$result; log=$log. No driver installed."
exit $result
