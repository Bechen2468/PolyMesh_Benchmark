#!/usr/bin/env python3
"""
Generate speedup-relative benchmark plots from Google Benchmark JSON output.

Usage:
    python plot_benchmarks.py results.json [more.json ...]

Re-running after adding a new matrix type or more benchmark runs needs no
code changes -- benchmark names are parsed automatically as long as they
follow "<System>/<Version>/<Operation>/<args...>".
"""

import json
import re
import sys
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt
from matplotlib.ticker import FuncFormatter

# ----------------------------------------------------------------------
# Config -- edit as needed
# ----------------------------------------------------------------------

OUTPUT_DIR = Path("plots")

BASELINE_SYSTEM = "OpenMesh"

# Raw system-name prefix (before the first "/") -> display label.
# Anything not listed falls back to stripping trailing underscores, so a
# newly added matrix type still gets a sane label with no edits needed here.
SYSTEM_LABELS = {
    "OpenMesh____": "OpenMesh",
    "SurfaceMesh_": "SurfaceMesh",
    "D_Ragged____": "PM-Reuse-Ragged",
    "Apd_Ragged__": "PM-Append-Ragged",
    "VecVec______": "PM-VecVec",
}

# Full_Load numeric value -> display label for the Load benchmark's x-axis.
FULL_LOAD_LABELS = {
    0: "Core Only",
    1: "Full",
    2: "VV-HE",
    3: "VF",
    4: "EF",
}

# Consistent color per system across every figure. Extend as needed.
SYSTEM_COLORS = {
    "OpenMesh":          "#4C72B0",
    "SurfaceMesh": "#DD8452",
    "PM-Reuse-Ragged":  "#55A868",
    "PM-Append-Ragged":     "#C44E52",
    #"PM-VecVec":     "#C44E52",        # TODO: needs color selected
}
DEFAULT_COLOR_CYCLE = plt.rcParams["axes.prop_cycle"].by_key()["color"]

# For every operation EXCEPT "Load", only this Full_Load value is plotted --
# the others are setup-state variants that don't apply to operations that
# don't care how the mesh got built.
REPRESENTATIVE_FULL_LOAD = 1

# Cheap serif match to LaTeX's default look, no LaTeX toolchain required.
# Set True for an exact Computer Modern match if `latex` is on this machine.
USE_TEX = False
if USE_TEX:
    plt.rcParams["text.usetex"] = True
plt.rcParams["font.family"] = "serif"
plt.rcParams["font.serif"] = ["cmr10", "Computer Modern Roman", "DejaVu Serif"]
plt.rcParams["axes.formatter.use_mathtext"] = True

# ----------------------------------------------------------------------
# Parsing
# ----------------------------------------------------------------------

NAME_RE = re.compile(
    r"^(?P<system>[^/]+)/(?P<version>[^/]+)/(?P<operation>[^/]+)/(?P<args>.+)$"
)
FULL_LOAD_RE = re.compile(r"Full_Load:(\d+)")


def clean_system_label(raw: str) -> str:
    return SYSTEM_LABELS.get(raw, raw.rstrip("_"))


def load_entries(json_paths):
    """Yield {system, operation, full_load, time} for every individual
    (non-aggregate) benchmark entry across all given JSON files."""
    for path in json_paths:
        with open(path) as f:
            data = json.load(f)
        for b in data.get("benchmarks", []):
            if b.get("run_type") == "aggregate":
                continue

            m = NAME_RE.match(b.get("name", ""))
            if not m:
                continue

            fl_m = FULL_LOAD_RE.search(m.group("args"))
            if not fl_m:
                continue

            yield {
                "system": clean_system_label(m.group("system")),
                "operation": m.group("operation"),
                "full_load": int(fl_m.group(1)),
                "time": b["real_time"],
            }


# ----------------------------------------------------------------------
# Plotting
# ----------------------------------------------------------------------

def plain_number(value, _pos):
    return f"{value:g}"

def system_color(system, idx):
    return SYSTEM_COLORS.get(system, DEFAULT_COLOR_CYCLE[idx % len(DEFAULT_COLOR_CYCLE)])


def get_baseline_time(entries, full_load):
    """Baseline time for the given Full_Load value; falls back to whichever
    single value the baseline system actually has (e.g. OpenMesh only ever
    runs Full_Load:1)."""
    candidates = [e for e in entries if e["system"] == BASELINE_SYSTEM]
    exact = [e for e in candidates if e["full_load"] == full_load]
    if exact:
        return exact[0]["time"]
    return candidates[0]["time"] if candidates else None

def save_all_formats(fig, out_dir, filename_stem):
    fig.savefig(out_dir / f"{filename_stem}.pdf")



def plot_operation(operation, entries, out_dir):
    subset = [e for e in entries if e["full_load"] == REPRESENTATIVE_FULL_LOAD]
    baseline_time = get_baseline_time(subset, REPRESENTATIVE_FULL_LOAD)
    if baseline_time is None:
        print(f"  [skip] {operation}: no baseline entry found")
        return

    other_systems = sorted({e["system"] for e in subset if e["system"] != BASELINE_SYSTEM})
    systems = [BASELINE_SYSTEM] + other_systems
    if not other_systems:
        print(f"  [skip] {operation}: no non-baseline entries found")
        return

    speedups = [1.0]
    for s in other_systems:
        matches = [e for e in subset if e["system"] == s]
        speedups.append(baseline_time / matches[0]["time"] if matches else np.nan)

    bar_width_in = 0.55
    fig_width = max(2.0, bar_width_in * len(systems) + 1.2)   # +1.2in for margins/labels
    fig, ax = plt.subplots(figsize=(fig_width, 2.6))

    x = np.arange(len(systems))
    bar_render_width = 0.6
    bars = ax.bar(x, speedups, color=[system_color(s, i) for i, s in enumerate(systems)], width=bar_render_width)
    ax.bar_label(bars, fmt="%.2f", fontsize=7, padding=2)

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(FuncFormatter(plain_number))
    ax.yaxis.set_minor_formatter(FuncFormatter(plain_number))
    ax.axhline(1.0, color="gray", linestyle="--", linewidth=0.7, zorder=0)
    ax.set_ylim(top=max(speedups) * 1.25)
    ax.set_xticks(x)
    ax.set_xticklabels(systems, rotation=20, ha="right", fontsize=8)
    ax.set_title(operation, fontsize=10)
    ax.tick_params(axis='y', labelsize=8)
    fig.tight_layout()

    save_all_formats(fig, out_dir, operation.lower())
    plt.close(fig)
    print(f"  [ok]   {operation} -> {out_dir / (operation.lower() + '.pdf')}")



def plot_load(entries, out_dir):
    subset = [e for e in entries if e["operation"] == "Load"]
    if not subset:
        print("  [skip] Load: no entries found")
        return

    full_load_values = sorted({e["full_load"] for e in subset})
    other_systems = sorted({e["system"] for e in subset if e["system"] != BASELINE_SYSTEM})
    systems = [BASELINE_SYSTEM] + other_systems
    if not other_systems:
        print("  [skip] Load: no non-baseline entries found")
        return

    n_groups = len(full_load_values)
    n_systems = len(systems)
    fig_width = max(3.0, 0.5 * n_systems * n_groups + 1.5)
    fig, ax = plt.subplots(figsize=(fig_width, 3.0))

    bar_width = 0.8 / n_systems
    x = np.arange(n_groups)
    all_speedups = []

    for i, s in enumerate(systems):
        speedups = []
        for fl in full_load_values:
            baseline_time = get_baseline_time(subset, fl)
            if s == BASELINE_SYSTEM:
                # OpenMesh only has data at the Full_Load value it actually
                # ran with -- NaN elsewhere so no bar is drawn there.
                has_data = any(e["system"] == s and e["full_load"] == fl for e in subset)
                speedups.append(1.0 if has_data else np.nan)
                continue
            matches = [e for e in subset if e["system"] == s and e["full_load"] == fl]
            speedups.append(baseline_time / matches[0]["time"] if baseline_time and matches else np.nan)
        all_speedups.extend(v for v in speedups if not np.isnan(v))
        offset = (i - (n_systems - 1) / 2) * bar_width
        bars = ax.bar(x + offset, speedups, width=bar_width, label=s, color=system_color(s, i))
        ax.bar_label(bars, fmt="%.2f", fontsize=7, padding=2)

    ax.set_yscale("log")
    ax.yaxis.set_major_formatter(FuncFormatter(plain_number))
    ax.yaxis.set_minor_formatter(FuncFormatter(plain_number))
    ax.axhline(1.0, color="gray", linestyle="--", linewidth=0.7, zorder=0)
    ax.set_ylim(top=max(all_speedups) * 1.25)
    ax.set_xticks(x)
    ax.set_xticklabels([FULL_LOAD_LABELS.get(v, str(v)) for v in full_load_values], fontsize=8)
    ax.set_title("Load", fontsize=10)
    ax.tick_params(axis='y', labelsize=8)
    ax.legend(loc="best", fontsize=7, ncol=2 if n_systems > 3 else 1)
    fig.tight_layout()

    save_all_formats(fig, out_dir, "load")
    plt.close(fig)
    print(f"  [ok]   Load -> {out_dir / 'load.pdf'}")
    
    
# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <benchmark_results.json> [more.json ...]")
        sys.exit(1)

    OUTPUT_DIR.mkdir(exist_ok=True)
    
    entries = list(load_entries(sys.argv[1:]))
    if not entries:
        print("No entries parsed -- check the JSON files and naming scheme.")
        sys.exit(1)

    operations = sorted({e["operation"] for e in entries if e["operation"] != "Load"})
    print(f"Found {len(entries)} entries, operations: {operations}, "
          f"systems: {sorted({e['system'] for e in entries})}")

    plot_load(entries, OUTPUT_DIR)
    for op in operations:
        plot_operation(op, [e for e in entries if e["operation"] == op], OUTPUT_DIR)


if __name__ == "__main__":
    main()