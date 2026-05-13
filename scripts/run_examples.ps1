<#
.SYNOPSIS
    Build (optional) and run every GradiumPP example, then report a pass/fail summary.

.DESCRIPTION
    Requirements:
      - GRADIUM_API_KEY environment variable must be set.
      - cmake + a C++17 compiler (only needed when -Build is passed).
      - ffmpeg (only needed for asr_realtime; detected at runtime).

.PARAMETER Build
    Run cmake configure + build before the examples.

.PARAMETER BuildDir
    Path to the CMake build directory (default: build).

.PARAMETER Voice
    Voice ID used for TTS examples (default: KRo-uwfno-KcEgBM / voices::en::american::abigail).

.PARAMETER AsrWav
    WAV file supplied to asr_rest. When omitted the script uses the WAV produced by tts_rest.

.PARAMETER AsrPcm
    Raw-PCM file (24 kHz, 16-bit, mono) supplied to asr_realtime. When omitted the script
    tries to convert the asr_rest WAV with ffmpeg; if ffmpeg is absent asr_realtime is skipped.

.PARAMETER Config
    CMake build configuration (Debug, Release, RelWithDebInfo, MinSizeRel).
    Defaults to Debug. Ignored for single-config generators (e.g. Ninja).

.PARAMETER Skip
    Comma-separated list of example names to skip.
    Valid names: voices tts_rest tts_realtime tts_multiplex asr_rest asr_realtime

.EXAMPLE
    .\scripts\run_examples.ps1
    .\scripts\run_examples.ps1 -Build -BuildDir build
    .\scripts\run_examples.ps1 -Config Release
    .\scripts\run_examples.ps1 -Skip tts_multiplex,asr_realtime
#>

[CmdletBinding()]
param(
    [switch]$Build,
    [string]$BuildDir  = 'build',
    [string]$Config    = 'Debug',
    [string]$Voice     = 'KRo-uwfno-KcEgBM',   # voices::en::american::abigail
    [string]$AsrWav    = '',
    [string]$AsrPcm    = '',
    [string]$Skip      = ''
)

$ErrorActionPreference = 'Stop'

# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------
$RepoRoot  = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDir  = Join-Path $RepoRoot $BuildDir

$Pass    = 0
$Fail    = 0
$SkipCnt = 0
$Results = @()

function Should-Skip([string]$Name) {
    if (-not $Skip) { return $false }
    $tokens = $Skip -split ','
    return $tokens -contains $Name
}

function Write-CapturedOutput([string]$Path) {
    # Read as UTF-8 (binary output) and split on all CR/LF variants so that
    # bare \r used for in-place terminal updates doesn't corrupt the display.
    $raw = [System.IO.File]::ReadAllText($Path, [System.Text.Encoding]::UTF8)
    $raw -split "`r`n|`r|`n" | Where-Object { $_ -ne '' } |
        ForEach-Object { Write-Host "         $_" }
}

function Run-Example {
    param(
        [string]  $Name,
        [string[]]$ExtraArgs   = @(),
        [switch]  $ShowOutput
    )

    # Multi-config generators (MSVC, Ninja Multi-Config) place the binary under
    # a Debug/Release/… subfolder; single-config generators (Ninja, Makefiles)
    # place it directly in the example directory.
    $BinMulti  = Join-Path $BuildDir "examples\$Name\$Config\gradium_$Name.exe"
    $BinSingle = Join-Path $BuildDir "examples\$Name\gradium_$Name.exe"
    $Bin = if (Test-Path $BinMulti) { $BinMulti } else { $BinSingle }

    if (Should-Skip $Name) {
        Write-Host "  [SKIP] $Name"
        $script:Results  += "SKIP  $Name"
        $script:SkipCnt++
        return
    }

    if (-not (Test-Path $Bin)) {
        Write-Host "  [FAIL] $Name -- binary not found: $Bin"
        $script:Results += "FAIL  $Name (binary not found)"
        $script:Fail++
        return
    }

    $ArgDisplay = ($ExtraArgs -join ' ')
    Write-Host "  Running: $Name $ArgDisplay"

    $TmpOut = [System.IO.Path]::GetTempFileName()
    try {
        & $Bin @ExtraArgs > $TmpOut 2>&1
        $ExitCode = $LASTEXITCODE
    } catch {
        $ExitCode = 1
        $_.Exception.Message | Out-File $TmpOut -Encoding utf8
    }

    if ($ExitCode -eq 0) {
        if ($ShowOutput) { Write-CapturedOutput $TmpOut }
        Write-Host "  [PASS] $Name"
        $script:Results += "PASS  $Name"
        $script:Pass++
    } else {
        Write-Host "  [FAIL] $Name (exit $ExitCode)"
        Write-Host "         --- output ---"
        Write-CapturedOutput $TmpOut
        $script:Results += "FAIL  $Name (exit $ExitCode)"
        $script:Fail++
    }

    Remove-Item $TmpOut -Force -ErrorAction SilentlyContinue
}

# ---------------------------------------------------------------------------
# Preflight checks
# ---------------------------------------------------------------------------
if (-not $env:GRADIUM_API_KEY) {
    Write-Error 'Error: GRADIUM_API_KEY is not set.'
    exit 1
}

# ---------------------------------------------------------------------------
# Optional build step
# ---------------------------------------------------------------------------
if ($Build) {
    Write-Host '==> Configuring and building...'
    cmake -B $BuildDir -S $RepoRoot -DGRADIUM_BUILD_EXAMPLES=ON
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    cmake --build $BuildDir --config $Config --parallel
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Write-Host ''
}

# ---------------------------------------------------------------------------
# Temporary working directory for generated files
# ---------------------------------------------------------------------------
$TmpDir = Join-Path ([System.IO.Path]::GetTempPath()) ([System.IO.Path]::GetRandomFileName())
New-Item -ItemType Directory -Path $TmpDir | Out-Null

try {

$TtsRestWav = Join-Path $TmpDir 'tts_output.wav'
$TtsRtWav   = Join-Path $TmpDir 'tts_realtime_output.wav'

Write-Host '==> Running examples...'
Write-Host ''

# 1. voices -- list only (no audio I/O)
Run-Example voices @('--list-only')

# 2. tts_rest
Run-Example tts_rest @(
    '--text', 'Hello from the GradiumPP integration script.',
    '--voice', $Voice,
    '--format', 'wav',
    '--out', $TtsRestWav
)

# 3. tts_realtime
Run-Example tts_realtime @(
    '--text', 'Hello from the GradiumPP realtime script.',
    '--voice', $Voice,
    '--format', 'wav',
    '--out', $TtsRtWav
)

# 4. tts_multiplex
# The binary writes to fixed filenames in the CWD; change into TmpDir first.
Push-Location $TmpDir
try {
    Run-Example tts_multiplex @('--voice', $Voice)
} finally {
    Pop-Location
}

# 5. asr_rest
if (-not $AsrWav -and (Test-Path $TtsRestWav)) {
    $AsrWav = $TtsRestWav
}

if ($AsrWav) {
    Run-Example asr_rest @('--file', $AsrWav, '--format', 'wav') -ShowOutput
} else {
    Write-Host '  [SKIP] asr_rest -- no WAV file available (tts_rest may have failed)'
    $Results  += 'SKIP  asr_rest (no WAV file)'
    $SkipCnt++
}

# 6. asr_realtime -- needs raw PCM (24 kHz, 16-bit, mono)
if (-not $AsrPcm) {
    $ffmpeg = Get-Command ffmpeg -ErrorAction SilentlyContinue
    if ($ffmpeg -and $AsrWav -and (Test-Path $AsrWav)) {
        $PcmFile = Join-Path $TmpDir 'asr_input.pcm'
        # Use cmd /c so PowerShell 5.1 never sees ffmpeg's stderr and never
        # raises a NativeCommandError regardless of $ErrorActionPreference.
        cmd /c "ffmpeg -y -i `"$AsrWav`" -ar 24000 -ac 1 -f s16le `"$PcmFile`" >nul 2>&1"
        if ($LASTEXITCODE -eq 0 -and (Test-Path $PcmFile)) {
            $AsrPcm = $PcmFile
        }
    }
}

if ($AsrPcm -and (Test-Path $AsrPcm)) {
    Run-Example asr_realtime @('--file', $AsrPcm, '--format', 'pcm') -ShowOutput
} else {
    Write-Host '  [SKIP] asr_realtime -- no raw-PCM file available (provide -AsrPcm or install ffmpeg)'
    $Results  += 'SKIP  asr_realtime (no PCM file; install ffmpeg or pass -AsrPcm)'
    $SkipCnt++
}

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
Write-Host ''
Write-Host '==> Results'
Write-Host '-------------------------------------------'
foreach ($r in $Results) { Write-Host "  $r" }
Write-Host '-------------------------------------------'
Write-Host "  Passed: $Pass  Failed: $Fail  Skipped: $SkipCnt"
Write-Host ''

} finally {
    Remove-Item $TmpDir -Recurse -Force -ErrorAction SilentlyContinue
}

if ($Fail -gt 0) { exit 1 }
