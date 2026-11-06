#!/usr/bin/env bash
set -e
trap 'sudo cpupower frequency-set --governor powersave' EXIT

# ----------------------------------------------------------------------
# Selection: a benchmark is run if its name contains one string from
# FILTERS_1 AND one string from FILTERS_2 (plain, case-sensitive substring).
# Use "" in a list to mean "anything" for that list.
#
# Every selected benchmark runs in its OWN process and writes its OWN JSON
# file, so a crash or out-of-memory kill only loses that one benchmark.
# ----------------------------------------------------------------------

# Keep the slashes: plain "Load" would also match "Full_Load" in every name.
FILTERS_1=(
    "/Load/"
    "/Render_Export/"
    "/Subdivision/"
    "/Collapse/"
)

FILTERS_2=(
    "OpenMesh____"
    "SurfaceMesh_"
    "Libigl______"
    "D_Ragged____"
    "Apd_Ragged__"
    "VecVec______"
    "Vec_Eigen___"
    "Apd_Ragged_V"
)

# Extra arguments passed to every run, e.g. "--benchmark_repetitions=3"
EXTRA_ARGS=()

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/.."

BENCH_BIN="./benchmark/PolyMesh_Benchmark"

cd "$REPO_ROOT/build_Release"

#clear
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# One folder per invocation, so it can be passed straight to the plot scripts.
RESULTS_DIR="$REPO_ROOT/build_Release/results/$(date +%Y-%m-%d_%H-%M-%S)"
mkdir -p "$RESULTS_DIR"

# Escape regex special characters so a name matches literally.
escape_regex() {
    printf '%s' "$1" | sed 's/[][\.|$(){}?+*^\\]/\\&/g'
}

# Turn a benchmark name into a safe file name.
safe_name() {
    local s
    s="$(printf '%s' "$1" | sed 's/[^A-Za-z0-9._-]/_/g; s/__*/_/g; s/^_//; s/_$//')"
    printf '%s' "${s:-unnamed}"
}

# ----------------------------------------------------------------------
# List all benchmarks once and select the ones to run
# ----------------------------------------------------------------------

echo "Listing benchmarks..."
set +e
LIST_OUTPUT="$("$BENCH_BIN" --benchmark_list_tests 2>&1)"
LIST_RC=$?
set -e

mapfile -t ALL_NAMES < <(printf '%s\n' "$LIST_OUTPUT" | grep -E '^[^[:space:]]+/[^[:space:]]+' || true)

if [ ${#ALL_NAMES[@]} -eq 0 ]; then
    echo "!!! The benchmark binary listed no benchmarks (exit code $LIST_RC). Output:"
    printf '%s\n' "$LIST_OUTPUT" | head -30 | sed 's/^/      /'
    exit 1
fi

echo "Found ${#ALL_NAMES[@]} benchmark(s)"

SELECTED=()
for name in "${ALL_NAMES[@]}"; do
    hit=0
    for f1 in "${FILTERS_1[@]}"; do
        [[ "$name" == *"$f1"* ]] || continue
        for f2 in "${FILTERS_2[@]}"; do
            if [[ "$name" == *"$f2"* ]]; then
                hit=1
                break 2
            fi
        done
    done
    [ $hit -eq 1 ] && SELECTED+=("$name")
done

if [ ${#SELECTED[@]} -eq 0 ]; then
    echo "!!! No benchmark matches the filters. Example name: ${ALL_NAMES[0]}"
    exit 1
fi

echo "Selected ${#SELECTED[@]} of ${#ALL_NAMES[@]} benchmark(s)"

# ----------------------------------------------------------------------
# Run each selected benchmark in its own process
# ----------------------------------------------------------------------

sudo cpupower frequency-set --governor performance

FAILED=()
total=${#SELECTED[@]}
i=0

for name in "${SELECTED[@]}"; do
    i=$((i + 1))
    out_file="$RESULTS_DIR/$(safe_name "$name").json"

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
    if ! grep -q '"run_name"\|"name"' "$out_file" 2>/dev/null; then
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