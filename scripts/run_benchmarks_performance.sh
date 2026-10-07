#!/usr/bin/env bash
set -e
trap 'sudo cpupower frequency-set --governor powersave' EXIT

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$SCRIPT_DIR/.."

cd "$REPO_ROOT/build_Release"

clear
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

sudo cpupower frequency-set --governor performance
./benchmark/PolyMesh_Benchmark --benchmark_format=json --benchmark_out=results.json
sudo cpupower frequency-set --governor powersave

cd "$REPO_ROOT"

