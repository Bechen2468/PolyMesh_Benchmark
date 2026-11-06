# Windows version of run_benchmarks.sh
#
# Run from PowerShell (e.g. the VS Code terminal):
#     .\run_benchmarks.ps1
# If scripts are blocked by the execution policy:
#     powershell -ExecutionPolicy Bypass -File .\run_benchmarks.ps1

# "Continue" on purpose: with "Stop", Windows PowerShell 5.1 turns any stderr
# output of a native program into a fatal error. Exit codes are checked by hand.
$ErrorActionPreference = "Continue"

# ----------------------------------------------------------------------
# One process is started for every combination of one string from
# $Filters1 and one string from $Filters2. It runs every benchmark whose
# name CONTAINS BOTH strings, and writes its own JSON file.
# Matching is literal: characters like | . ( ) are escaped automatically.
# Use "" in a list to mean "anything" for that list.
# ----------------------------------------------------------------------

# Keep the slashes: plain "Load" would also match "Full_Load" in every name.
$Filters1 = @(
    "/Memory_Usage/"
    "/Load/"
    "/Circular/"
    "/Barycenter/"
    "/Normal/"
    "/Smoothing/"
    "/Subdivision/"
    "/Collapse/"
    "/Render_Export/"
)

$Filters2 = @(
    "OpenMesh____"
    "SurfaceMesh_"
    "Libigl______"
    "D_Ragged____"
    "Apd_Ragged__"
    "VecVec______"
    "Vec_Eigen___"
    "Apd_Ragged_V"
)

# Extra arguments passed to every run, e.g. @("--benchmark_repetitions=3")
$ExtraArgs = @()

# Windows "High performance" power plan (replaces cpupower on Linux).
$HighPerfPlan = "8c5e7fda-e8bf-4a96-9a85-a6e23a8c635c"

# ----------------------------------------------------------------------

$RepoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$BuildDir = Join-Path $RepoRoot "build_Release"

New-Item -ItemType Directory -Force -Path $BuildDir | Out-Null
Set-Location $BuildDir

cmake .. -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($LASTEXITCODE)" }

cmake --build . --config Release
if ($LASTEXITCODE -ne 0) { throw "cmake build failed ($LASTEXITCODE)" }

# Visual Studio generators put the exe in a Release\ subfolder, Ninja/Make don't.
$BenchBin = @(
    (Join-Path $BuildDir "benchmark\Release\PolyMesh_Benchmark.exe")
    (Join-Path $BuildDir "benchmark\PolyMesh_Benchmark.exe")
) | Where-Object { Test-Path $_ } | Select-Object -First 1

if (-not $BenchBin) { throw "PolyMesh_Benchmark.exe not found under $BuildDir\benchmark" }
Write-Host "Using $BenchBin"

# One folder per invocation, so it can be passed straight to the plot scripts.
$ResultsDir = Join-Path $BuildDir ("results\" + (Get-Date -Format "yyyy-MM-dd_HH-mm-ss"))
New-Item -ItemType Directory -Force -Path $ResultsDir | Out-Null

# Escape regex special characters so the filter matches the string literally.
function Escape-Regex([string]$s) {
    return ($s -replace '([\[\]\\.|$(){}?+*^])', '\$1')
}

# Regex that matches exactly the given benchmark names and nothing else.
function Names-Regex([string[]]$names) {
    $parts = $names | ForEach-Object { Escape-Regex $_ }
    return "^(" + ($parts -join "|") + ")$"
}

# Turn a filter string into a safe file name.
function Safe-Name([string]$s) {
    $n = ($s -replace '[^A-Za-z0-9._-]', '_') -replace '_+', '_'
    $n = $n.Trim('_')
    if ($n) { return $n } else { return "all" }
}

function Describe-ExitCode([int]$code) {
    $hex = "0x{0:X8}" -f $code
    switch ($hex) {
        "0xC0000005" { return "$hex access violation (segfault)" }
        "0xC00000FD" { return "$hex stack overflow" }
        "0xC0000409" { return "$hex abort / buffer overrun (often std::bad_alloc = out of memory)" }
        "0xC0000017" { return "$hex out of memory" }
        "0xC0000135" { return "$hex a required DLL was not found (add its folder to PATH or copy it next to the exe)" }
        "0xC000007B" { return "$hex DLL mismatch (32/64-bit or Debug/Release mixed)" }
        default {
            if ($code -eq 3) { return "3 (abort(), often an uncaught exception such as std::bad_alloc)" }
            return "$code ($hex)"
        }
    }
}

# Remember the current power plan so it can be restored afterwards.
$OriginalPlan = $null
$m = [regex]::Match((powercfg /getactivescheme | Out-String), '[0-9a-fA-F-]{36}')
if ($m.Success) { $OriginalPlan = $m.Value }

# List every benchmark once. Matching is then done here in PowerShell, so it
# does not depend on the benchmark's regex engine.
Write-Host "Listing benchmarks..."
$listOutput = & $BenchBin --benchmark_list_tests 2>&1 | ForEach-Object { "$_" }
$listRc = $LASTEXITCODE
$AllNames = @($listOutput | Where-Object { $_ -match '^\S+/\S+' } | ForEach-Object { $_.Trim() })

if ($AllNames.Count -eq 0) {
    Write-Host ""
    Write-Host "!!! The benchmark binary listed no benchmarks, so nothing can be matched." -ForegroundColor Red
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

Write-Host "Found $($AllNames.Count) benchmark(s), e.g. $($AllNames[0])"

$Failed = @()
$Skipped = @()

try {
    powercfg /setactive $HighPerfPlan 2>$null
    if ($LASTEXITCODE -ne 0) {
        Write-Warning "Could not switch to the High performance power plan; running with the current plan."
    }

    foreach ($f1 in $Filters1) {
        foreach ($f2 in $Filters2) {
            $combo = "`"$f1`" + `"$f2`""

            # Plain, case-sensitive substring match (same as before, but done here).
            $matched = @($AllNames | Where-Object { $_.Contains($f1) -and $_.Contains($f2) })
            $nMatches = $matched.Count

            # Skip combinations that match no benchmark (no empty JSON files).
            if ($nMatches -eq 0) {
                Write-Host "--- ${combo}: no matching benchmarks, skipped"
                $Skipped += "$f1+$f2"
                continue
            }

            $outFile = Join-Path $ResultsDir ("{0}__{1}.json" -f (Safe-Name $f1), (Safe-Name $f2))

            $regex = Names-Regex $matched

            Write-Host ""
            Write-Host "=== ${combo}: $nMatches benchmark(s) -> $outFile"

            # Run in its own process. A crash or out-of-memory only loses this
            # combination; the loop continues with the next one.
            # Console shows the normal table, the JSON goes to the file.
            & $BenchBin `
                "--benchmark_filter=$regex" `
                "--benchmark_out=$outFile" `
                --benchmark_out_format=json `
                @ExtraArgs
            $rc = $LASTEXITCODE

            if ($rc -ne 0) {
                Write-Host "!!! $combo failed: $(Describe-ExitCode $rc)" -ForegroundColor Red
                $Failed += "$f1+$f2"
                continue
            }

            # Sanity check: did the run actually execute benchmarks?
            $ran = 0
            if (Test-Path $outFile) {
                try {
                    $json = Get-Content $outFile -Raw | ConvertFrom-Json
                    $ran = @($json.benchmarks).Count
                } catch { $ran = 0 }
            }
            if ($ran -eq 0) {
                Write-Host "!!! $combo ran no benchmarks although $nMatches name(s) matched." -ForegroundColor Red
                Write-Host "    Filter passed to the binary: $regex"
                $Failed += "$f1+$f2"
            }
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

if ($Skipped.Count -gt 0) {
    Write-Host "Skipped (no match): $($Skipped -join ' ')"
}

if ($Failed.Count -gt 0) {
    Write-Host "Failed: $($Failed -join ' ')" -ForegroundColor Red
    exit 1
}