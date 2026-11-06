#!/usr/bin/env bash
#
# Builds the project and runs EVERY benchmark the binary lists, each in its
# own process, each writing its own JSON file into one timestamped folder.
# A crash or out-of-memory kill only loses that one benchmark.
#
# Works on Linux and macOS (macOS ships bash 3.2, so no bash-4 features here).

set -e

# Extra arguments passed to every run, e.g. EXTRA_ARGS=(--benchmark_repetitions=3)
EXTRA_ARGS=()

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$REPO_ROOT/build_Release"

BENCH_BIN="./benchmark/PolyMesh_Benchmark"

# ----------------------------------------------------------------------
# CPU frequency governor (Linux only; macOS has no equivalent)
# ----------------------------------------------------------------------

USE_CPUPOWER=0
if [ "$(uname -s)" = "Linux" ] && command -v cpupower >/dev/null 2>&1; then
    USE_CPUPOWER=1
    trap 'sudo cpupower frequency-set --governor powersave >/dev/null' EXIT
else
    echo "Note: cpupower not available ($(uname -s)), CPU governor is left unchanged."
fi

# ----------------------------------------------------------------------
# Build
# ----------------------------------------------------------------------

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

#clear
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

if [ ! -x "$BENCH_BIN" ]; then
    echo "!!! Benchmark binary not found: $BUILD_DIR/${BENCH_BIN#./}"
    exit 1
fi

# One folder per invocation, so it can be passed straight to the plot scripts.
RESULTS_DIR="$BUILD_DIR/results/$(date +%Y-%m-%d_%H-%M-%S)"
mkdir -p "$RESULTS_DIR"

# Escape regex special characters so a name matches literally.
escape_regex() {
    printf '%s' "$1" | sed 's/[][\.|$(){}?+*^\\]/\\&/g'
}

# Turn a benchmark name into a safe file name.
safe_name() {
    local s
    s="$(printf '%s' "$1" | sed -e 's/[^A-Za-z0-9._-]/_/g' -e 's/__*/_/g' -e 's/^_//' -e 's/_$//')"
    printf '%s' "${s:-unnamed}"
}

# ----------------------------------------------------------------------
# List all benchmarks once
# ----------------------------------------------------------------------

echo "Listing benchmarks..."
LIST_ERR="$(mktemp "${TMPDIR:-/tmp}/bench_list.XXXXXX")"

set +e
LIST_OUTPUT="$("$BENCH_BIN" --benchmark_list_tests 2>"$LIST_ERR")"
LIST_RC=$?
set -e

ALL_NAMES=()
while IFS= read -r line; do
    case "$line" in
        */*) ALL_NAMES+=("$line") ;;   # benchmark names contain a "/"
    esac
done <<EOF
$LIST_OUTPUT
EOF

if [ ${#ALL_NAMES[@]} -eq 0 ]; then
    echo "!!! The benchmark binary listed no benchmarks (exit code $LIST_RC). Output:"
    { printf '%s\n' "$LIST_OUTPUT"; cat "$LIST_ERR"; } | head -30 | sed 's/^/      /'
    rm -f "$LIST_ERR"
    exit 1
fi
rm -f "$LIST_ERR"

total=${#ALL_NAMES[@]}
echo "Found $total benchmark(s)"

# ----------------------------------------------------------------------
# Run each benchmark in its own process
# ----------------------------------------------------------------------

if [ $USE_CPUPOWER -eq 1 ]; then
    sudo cpupower frequency-set --governor performance >/dev/null
fi

FAILED=()
i=0

for name in "${ALL_NAMES[@]}"; do
    i=$((i + 1))
    # Index prefix keeps files unique and in run order.
    out_file="$RESULTS_DIR/$(printf '%04d' "$i")_$(safe_name "$name").json"

    echo
    echo "=== [$i/$total] $name"

    set +e
    "$BENCH_BIN" \
        --benchmark_filter="^$(escape_regex "$name")\$" \
        --benchmark_out="$out_file" \
        --benchmark_out_format=json \
        "${EXTRA_ARGS[@]}"
    rc=$?
    set -e

    if [ $rc -ne 0 ]; then
        if [ $rc -eq 137 ]; then
            echo "!!! killed (exit 137, most likely out of memory)"
        else
            echo "!!! failed (exit code $rc)"
        fi
        FAILED+=("$name")
        continue
    fi

    # Sanity check: the JSON must contain a result for this benchmark.
    if ! grep -q '"run_name"' "$out_file" 2>/dev/null; then
        echo "!!! ran but produced no result"
        FAILED+=("$name")
    fi
done

cd "$REPO_ROOT"

echo
echo "Results in: $RESULTS_DIR"
echo "Succeeded: $((total - ${#FAILED[@]})) / $total"

if [ ${#FAILED[@]} -gt 0 ]; then
    echo "Failed:"
    printf '  %s\n' "${FAILED[@]}"
    exit 1
fi