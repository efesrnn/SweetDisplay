#requires -Version 7.0
param([string]$Repository = (Split-Path (Split-Path $PSScriptRoot -Parent) -Parent))
$ErrorActionPreference = 'Stop'
$Repository = (Resolve-Path -LiteralPath $Repository).Path
$findings = [Collections.Generic.List[string]]::new()
$seenBlobs = [Collections.Generic.HashSet[string]]::new()

function Invoke-GitBytes([string[]]$Arguments) {
    $info = [Diagnostics.ProcessStartInfo]::new('git')
    $info.WorkingDirectory = $Repository
    $info.UseShellExecute = $false
    $info.RedirectStandardOutput = $true
    $info.RedirectStandardError = $true
    $info.CreateNoWindow = $true
    $info.ArgumentList.Add('--no-optional-locks')
    foreach ($argument in $Arguments) { $info.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($info)
    $errorTask = $process.StandardError.ReadToEndAsync()
    $stream = [IO.MemoryStream]::new()
    try {
        $process.StandardOutput.BaseStream.CopyTo($stream)
        $process.WaitForExit()
        $null = $errorTask.GetAwaiter().GetResult()
        if ($process.ExitCode -ne 0) { throw 'Git inspection failed; publication check cannot pass.' }
        return ,$stream.ToArray()
    } finally { $stream.Dispose(); $process.Dispose() }
}
function Git-Text([string[]]$Arguments) {
    return [Text.Encoding]::UTF8.GetString((Invoke-GitBytes $Arguments))
}
function Check-Path([string]$Path, [string]$Origin) {
    if ($Path -match '[\x00-\x1f]' -or $Path -match '(^|/)(out|build|downloads|firmware|upstream|private|\.local|\.codex|\.agents|\.vs|\.vscode|\.idea|x64|ARM64|Debug|Release|bin|obj|__pycache__)(/|$)' -or
        ($Path.StartsWith('docs/evidence/') -and $Path -ne 'docs/evidence/README.md') -or
        $Path -match '(^|/)(\.env($|\.)|id_rsa|id_ed25519|credentials\.|secrets\.)' -or
        $Path -match '\.local\.' -or
        $Path -match '\.(iso|img|bin|mbn|elf|dtbo?|fw|exe|dll|sys|cat|pdb|obj|lib|exp|ilk|pch|res|log|binlog|etl|dmp|jsonl|pem|key|pfx|p12|p8|cer|crt|der|jks|keystore|zip|7z|rar|tar|gz|tgz|xz|zst|lz4|cab|msi|msix|vsix)$') {
        $findings.Add("$Origin : excluded/private/generated path : $Path")
    }
}
function Check-Bytes([byte[]]$Bytes, [string]$Path, [string]$Origin) {
    if ($Bytes.Length -gt 1MB) { $findings.Add("$Origin : exceeds 1 MiB review limit : $Path"); return }
    # Fail closed on unknown binary payloads; UTF-16/UTF-8 text is decoded first.
    if ($Bytes.Length -ge 2 -and $Bytes[0] -eq 255 -and $Bytes[1] -eq 254) {
        $value = [Text.Encoding]::Unicode.GetString($Bytes, 2, $Bytes.Length - 2)
    } elseif ($Bytes.Length -ge 2 -and $Bytes[0] -eq 254 -and $Bytes[1] -eq 255) {
        $value = [Text.Encoding]::BigEndianUnicode.GetString($Bytes, 2, $Bytes.Length - 2)
    } else {
        try { $value = [Text.UTF8Encoding]::new($false, $true).GetString($Bytes) }
        catch { $findings.Add("$Origin : non-text payload needs review : $Path"); return }
    }
    if ($value.Contains([char]0)) { $findings.Add("$Origin : binary/NUL payload needs review : $Path"); return }
    $patterns = [ordered]@{
        'private key' = '-----BEGIN (?:[A-Z0-9 ]+ )?PRIVATE KEY-----'
        'provider token' = '(?:gh[pousr]_[A-Za-z0-9]{20,}|github_pat_[A-Za-z0-9_]{20,}|AKIA[0-9A-Z]{16}|xox[baprs]-[A-Za-z0-9-]{15,}|sk-(?:proj-|svcacct-)?[A-Za-z0-9_-]{20,}|AIza[0-9A-Za-z_-]{30,})'
        'JWT credential' = 'eyJ[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}\.[A-Za-z0-9_-]{10,}'
        'credential URL' = 'https?://[^\s/:@]+:[^\s/@]+@'
        'user home path' = '(?i)([A-Z]:[\\/]+Users[\\/]+[^\s\\/<>"'']+|/(?:home|Users)/[^\s/<>"'']+)'
        'email address' = '(?i)\b[A-Z0-9._%+-]+@[A-Z0-9.-]+\.[A-Z]{2,}\b'
        'literal credential assignment' = '(?i)["'']?(?:api[_-]?key|access[_-]?token|client[_-]?secret|password|device[_-]?serial|account[_-]?id)["'']?\s*[:=]\s*["''](?!<|\$|YOUR_|EXAMPLE|REDACTED|CHANGEME)[A-Za-z0-9_./+=-]{8,}["'']'
    }
    foreach ($rule in $patterns.GetEnumerator()) {
        if ($value -match $rule.Value) { $findings.Add("$Origin : $($rule.Key) : $Path") }
    }
    if ($env:USERNAME -and $env:USERNAME.Length -ge 4 -and
        $value -match ('(?i)(?<![\w])' + [regex]::Escape($env:USERNAME) + '(?![\w])')) {
        $findings.Add("$Origin : local username : $Path")
    }
}
function Check-Blob([string]$Hash, [string]$Path, [string]$Origin) {
    Check-Path $Path $Origin
    if (-not $seenBlobs.Add($Hash)) { return }
    $size = [long](Git-Text @('cat-file', '-s', $Hash)).Trim()
    if ($size -gt 1MB) { $findings.Add("$Origin : blob exceeds 1 MiB review limit : $Path"); return }
    Check-Bytes (Invoke-GitBytes @('cat-file', 'blob', $Hash)) $Path $Origin
}

$paths = @((Git-Text @('ls-files', '-z', '--cached', '--others', '--exclude-standard')).Split([char]0) | Where-Object { $_ } | Sort-Object -Unique)
foreach ($path in $paths) {
    Check-Path $path 'working tree'
    $full = Join-Path $Repository $path
    if (Test-Path -LiteralPath $full -PathType Leaf) {
        $file = Get-Item -LiteralPath $full -Force
        if ($file.Attributes -band [IO.FileAttributes]::ReparsePoint) { $findings.Add("working tree : link needs review : $path"); continue }
        if ($file.Length -gt 1MB) { $findings.Add("working tree : exceeds 1 MiB review limit : $path"); continue }
        Check-Bytes ([IO.File]::ReadAllBytes($full)) $path 'working tree'
    }
}
# Inspect actual index blobs too: a clean working copy can hide a staged secret.
foreach ($entry in (Git-Text @('ls-files', '--stage', '-z')).Split([char]0)) {
    if (-not $entry) { continue }
    if ($entry -notmatch '^(\d+) ([0-9a-f]+) (\d)\t([\s\S]+)$') { throw 'Unexpected index record.' }
    $mode, $hash, $stage, $path = $Matches[1], $Matches[2], $Matches[3], $Matches[4]
    if ($mode -notin '100644','100755' -or $stage -ne '0') { $findings.Add("index : link/submodule/conflict needs review : $path"); continue }
    Check-Blob $hash $path 'index'
}
# All local refs, including tags: ignoring a file does not remove committed copies.
$historyCount = 0
foreach ($entry in (Git-Text @('-c', 'core.quotePath=false', 'rev-list', '--objects', '--all')).Split("`n")) {
    if ($entry -notmatch '^([0-9a-f]+) (.+)$') { continue }
    $hash, $path = $Matches[1], $Matches[2].TrimEnd("`r")
    if ((Git-Text @('cat-file', '-t', $hash)).Trim() -ne 'blob') { continue }
    $historyCount++
    Check-Blob $hash $path 'history'
}
if ($findings.Count) {
    $findings | Sort-Object -Unique | ForEach-Object { Write-Output "FAIL: $_" }
    Write-Output 'Publication check failed. Values intentionally omitted; do not commit or push.'
    exit 1
}
Write-Output "PASS: $($paths.Count) publishable working-tree files; index and $historyCount reachable history blobs checked."
Write-Output 'Pattern checks are not a guarantee: review intended changes; never force-add ignored content.'
exit 0
