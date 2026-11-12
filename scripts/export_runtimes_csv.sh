#!/usr/bin/env bash
set -e


# Plot
source venv/bin/activate
python scripts/_export_runtimes_csv.py results/results_thinkpad results/results_windows results/results_apple
