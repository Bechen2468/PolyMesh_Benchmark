#!/usr/bin/env bash
set -e


# Plot
source venv/bin/activate
python scripts/_plot_benchmarks_compare.py results/results_thinkpad results/results_windows results/results_apple
