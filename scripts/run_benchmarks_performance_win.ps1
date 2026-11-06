# Windows version of run_benchmarks.sh
#
# Builds the project and runs EVERY benchmark the binary lists, each in its
# own process, each writing its own JSON file into one timestamped folder.
# A crash or out-of-memory only loses that one benchmark.
#
# Run from PowerShell (e.g. the VS Code terminal):
#     .\run_benchmarks.ps1
# If scripts are blocked by the execution policy:
#     powershell -ExecutionPolicy Bypass -File .\run_benchmarks.ps1

# "Continue" on purpose: with "Stop", Windows PowerShell 5.1 turns any stderr
# output of a native program into a fatal error. Exit codes are checked by hand.
$ErrorActionPreference = "Continue"

# Extra arguments passed to every run, e.g. @("--benchmark_repetitions=3")
$ExtraArgs = @()

# Windows "High performance" power plan (replaces cpupower on Linux).
$HighPerfPlan = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"

# ----------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------

$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDir = Join-Path $RepoRoot "build_Release"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Set-Location $BuildDir

cmake .. -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { Set-Location $RepoRoot; throw "cmake configure failed ($LASTEXITCODE)" }

cmake --build . --config Release
if ($LASTEXITCODE -ne 0) { Set-Location $RepoRoot; throw "cmake build failed ($LASTEXITCODE)" }

# Visual Studio generators put the exe in a Release\ subfolder, Ninja/Make don't.
$BenchBin = @(
    (Join-Path $BuildDir "benchmark\Release\PolyMesh_Benchmark.exe")
    (Join-Path $BuildDir "benchmark\PolyMesh_Benchmark.exe")
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $BenchBin) { Set-Location $RepoRoot; throw "PolyMesh_Benchmark.exe not found under $BuildDir\benchmark" }
Write-Host "Using $BenchBin"

# One folder per invocation, so it can be passed straight to the plot scripts.
$ResultsDir = Join-Path $BuildDir ("results\" + (Get-Date -Format "yyyy-MM-dd_HH-mm-ss"))
New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null

# Escape regex special characters so a name matches literally.
function Escape-Regex([string]$s) {
    return ($s -replace '([\[\]\\.|$(){}?+*^])', '\$1')
}

# Turn a benchmark name into a safe file name.
function Safe-Name([string]$s) {
    $n = ($s -replace '[^A-Za-z0-9._-]', '_') -replace '_+', '_'
    $n = $n.Trim('_')
    if ($n) { return $n } else { return "unnamed" }
}

function Describe-ExitCode([int]$code) {
    $hex = "0x{0:X8}" -f $code
    switch ($hex) {
        "0xC0000005" { return "$hex access violation (segfault)" }
        "0xC00000FD" { return "$hex stack overflow" }
        "0xC0000409" { return "$hex abort / buffer overrun (often std::bad_alloc = out of memory)" }
        "0xC0000017" { return "$hex out of memory" }
        "0xC000001D" { return "$hex illegal instruction (CPU lacks e.g. AVX2)" }
        "0xC0000135" { return "$hex a required DLL was not found" }
        "0xC000007B" { return "$hex DLL mismatch (32/64-bit or Debug/Release mixed)" }
        default {
            if ($code -eq 3) { return "3 (abort(), often an uncaught exception such as std::bad_alloc)" }
            return "$code ($hex)"
        }
    }
}

# ----------------------------------------------------------------------
# List all benchmarks once
# ----------------------------------------------------------------------

Write-Host "Listing benchmarks..."
$listOutput = & $BenchBin --benchmark_list_tests 2>&1 | ForEach-Object { "$_" }
$listRc = $LASTEXITCODE
# Benchmark names contain a "/"; this skips any other output.
$AllNames = @($listOutput | Where-Object { $_ -match '^\S+/\S+' } | ForEach-Object { $_.Trim() })

if ($AllNames.Count -eq 0) {
    Write-Host ""
    Write-Host "!!! The benchmark binary listed no benchmarks." -ForegroundColor Red
    Write-Host "    Exit code: $(Describe-ExitCode $listRc)"
    Write-Host "    Output of '$BenchBin --benchmark_list_tests':"
    if ($listOutput) {
        $listOutput | Select-Object -First 30 | ForEach-Object { Write-Host "      $_" }
    } else {
        Write-Host "      (nothing)"
    }
    Set-Location $RepoRoot
    exit 1
}

$Total = $AllNames.Count
Write-Host "Found $Total benchmark(s)"

# ----------------------------------------------------------------------
# Run each benchmark in its own process
# ----------------------------------------------------------------------

# Remember the current power plan so it can be restored afterwards.
$OriginalPlan = $null
$m = [regex]::Match((powercfg /getactivescheme | Out-String), '[0-9a-fA-F-]{36}')
if ($m.Success) { $OriginalPlan = $m.Value }

$Failed = @()

try {
    powercfg /setactive $HighPerfPlan 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Could not switch to the High performance power plan; running with the current plan."
    }

    $i = 0
    foreach ($name in $AllNames) {
        $i++
        # Index prefix keeps files unique and in run order.
        $outFile = Join-Path $ResultsDir ("{0:D4}_{1}.json" -f $i, (Safe-Name $name))

        Write-Host ""
        Write-Host "=== [$i/$Total] $name"

        & $BenchBin `
            "--benchmark_filter=^$(Escape-Regex $name)$" `
            "--benchmark_out=$outFile" `
            --benchmark_out_format=json `
            @ExtraArgs
        $rc = $LASTEXITCODE

        if ($rc -ne 0) {
            Write-Host "!!! failed: $(Describe-ExitCode $rc)" -ForegroundColor Red
            $Failed += $name
            continue
        }

        # Sanity check: the JSON must contain a result for this benchmark.
        $ran = 0
        if (Test-Path $outFile) {
            try {
                $json = Get-Content $outFile -Raw | ConvertFrom-Json
                $ran = @($json.benchmarks).Count
            } catch { $ran = 0 }
        }
        if ($ran -eq 0) {
            Write-Host "!!! ran but produced no result" -ForegroundColor Red
            $Failed += $name
        }
    }
}
finally {
    if ($OriginalPlan) {
        powercfg /setactive $OriginalPlan 2>$null
    }
    Set-Location $RepoRoot
}

Write-Host ""
Write-Host "Results in: $ResultsDir"
Write-Host "Succeeded: $($Total - $Failed.Count) / $Total"

if ($Failed.Count -gt 0) {
    Write-Host "Failed:" -ForegroundColor Red
    $Failed | ForEach-Object { Write-Host "  $_" }
    exit 1
}