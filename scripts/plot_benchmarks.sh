#!/usr/bin/env bash
set -e


# Plot
source venv/bin/activate
python scripts/_plot_benchmarks.py build_Release/results.json






