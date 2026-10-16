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
from matplotlib.ticker import FuncFormatter, LogLocator, NullFormatter

# ----------------------------------------------------------------------
# Config -- edit as needed
# ----------------------------------------------------------------------

OUTPUT_DIR = Path("plots")

BASELINE_SYSTEM = "OpenMesh"

# Raw system-name prefix (before the first "/") -> display label.
SYSTEM_LABELS = {
    "OpenMesh____": "OpenMesh",
    "SurfaceMesh_": "SurfaceMesh",
    "Libigl______": "Libigl",
    "D_Ragged____": "PM-Reuse-Ragged",
    "Apd_Ragged__": "PM-Append-Ragged",
    "VecVec______": "PM-VecVec",
    "Vec_Eigen___": "PM-VecEigen",
    "Apd_Ragged_V": "PM-Validate",
}
SYSTEM_ORDER = list(SYSTEM_LABELS.values())

# Full_Load numeric value -> display label for the Load benchmark's x-axis.
FULL_LOAD_LABELS = {
    0: "Core Only",
    2: "VV-HE",
    3: "VF",
    4: "EF",
    1: "Full",
    10: "VALIDATE",
}

# Consistent color per system across every figure.
# SYSTEM_COLORS = {
#     "OpenMesh": "#595959",
#     "SurfaceMesh": "#f0c571",
#     "Libigl": "#008dff",
#     "PM-Reuse-Ragged": "#9d2c00",
#     "PM-Append-Ragged": "#0b81a2",
#     "PM-VecVec": "#e25759",
#     "PM-VecEigen": "#7E4794",
#     "PM-Validate": "#36b700",
# }
SYSTEM_COLORS = {
    "OpenMesh":         "#595959",   # neutral baseline
    "SurfaceMesh":      "#2a78d6",   # blue
    "Libigl":           "#eb6834",   # orange
    "PM-Reuse-Ragged":  "#1baf7a",   # aqua
    "PM-Append-Ragged": "#eda100",   # yellow
    "PM-VecVec":        "#e87ba4",   # magenta
    "PM-VecEigen":      "#008300",   # green
    "PM-Validate":      "#6250d6",   # violet
}

DEFAULT_COLOR_CYCLE = plt.rcParams["axes.prop_cycle"].by_key()["color"]

# For every operation EXCEPT "Load", only this Full_Load value is plotted.
REPRESENTATIVE_FULL_LOAD = [1, 10]

# ----------------------------------------------------------------------
# Plot appearance
# ----------------------------------------------------------------------

# Width occupied by all bars belonging to one x-axis group.
# 0.8 means 80% of the distance between two groups.
GROUP_WIDTH = 0.70

# Maximum number of labeled major ticks on the y-axis.
MAX_Y_TICKS = 5

# How much vertical room to leave above the largest bar.
Y_TOP_MARGIN = 1.30

# How much room to leave below the smallest bar.
Y_BOTTOM_MARGIN = 1.25



# Figure sizing
LOAD_FIG_HEIGHT = 3.1
OPERATION_FIG_HEIGHT = 2.8

# Width per system for non-Load plots.
# Smaller = narrower graphs.
OPERATION_WIDTH_PER_SYSTEM = 0.45

# Minimum total width of a non-Load plot.
OPERATION_MIN_FIG_WIDTH = 2.4

# Extra width for y-axis, margins, etc.
OPERATION_EXTRA_WIDTH = 1.0




USE_TEX = False
if USE_TEX:
    plt.rcParams["text.usetex"] = True

plt.rcParams["font.family"] = "serif"
plt.rcParams["font.serif"] = [
    "cmr10",
    "Computer Modern Roman",
    "DejaVu Serif",
]
plt.rcParams["axes.formatter.use_mathtext"] = True


# ----------------------------------------------------------------------
# Parsing
# ----------------------------------------------------------------------

NAME_RE = re.compile(
    r"^(?P<system>[^/]+)/(?P<version>[^/]+)/"
    r"(?P<operation>[^/]+)/(?P<args>.+)$"
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
# Plot helpers
# ----------------------------------------------------------------------

def order_systems(systems):
    return sorted(
        systems,
        key=lambda s: (
            SYSTEM_ORDER.index(s) if s in SYSTEM_ORDER else len(SYSTEM_ORDER),
            s,
        ),
    )


def plain_number(value, _pos=None):
    """Compact numeric formatting for axis labels."""
    if value == 0:
        return "0"

    # Avoid things like 0.100000 or 1000.0.
    if abs(value) >= 1000 or abs(value) < 0.01:
        return f"{value:.1e}"

    if value >= 10:
        return f"{value:.0f}"

    if value >= 1:
        return f"{value:.1f}"

    return f"{value:.2f}"


def system_color(system, idx):
    return SYSTEM_COLORS.get(
        system,
        DEFAULT_COLOR_CYCLE[idx % len(DEFAULT_COLOR_CYCLE)],
    )


def get_baseline_time(entries, full_load):
    """Baseline time for the given Full_Load value.

    Falls back to whichever single value the baseline system actually has.
    """

    candidates = [
        e for e in entries
        if e["system"] == BASELINE_SYSTEM
    ]

    exact = [
        e for e in candidates
        if e["full_load"] == full_load
    ]

    if exact:
        return exact[0]["time"]

    return candidates[0]["time"] if candidates else None


def configure_log_y_axis(ax, values):
    """
    Configure a readable logarithmic y-axis based on the actual data.

    Important:
      - Only major ticks receive labels.
      - Minor ticks remain unlabeled.
      - Limits adapt to both the smallest and largest values.
      - The range always contains 1.0 because speedups are relative
        to the baseline.
    """

    values = [
        float(v)
        for v in values
        if v is not None and np.isfinite(v) and v > 0
    ]

    if not values:
        return

    data_min = min(values)
    data_max = max(values)

    # Speedups are relative to the baseline, so 1.0 is important.
    data_min = min(data_min, 1.0)
    data_max = max(data_max, 1.0)

    # Work in log space so that margins are visually symmetric.
    log_min = np.log10(data_min)
    log_max = np.log10(data_max)

    # Add margin around the data.
    log_min -= np.log10(Y_BOTTOM_MARGIN)
    log_max += np.log10(Y_TOP_MARGIN)

    # Do not let the axis become absurdly narrow around 1.
    if log_max - log_min < 0.6:
        center = (log_min + log_max) / 2
        log_min = center - 0.3
        log_max = center + 0.3

    ax.set_ylim(10 ** log_min, 10 ** log_max)

    # Major ticks at powers of 10.
    major_locator = LogLocator(
        base=10,
        subs=(1.0, 2.0, 5.0),
        numticks=MAX_Y_TICKS,
    )

    ax.yaxis.set_major_locator(major_locator)
    ax.yaxis.set_major_formatter(
        FuncFormatter(plain_number)
    )

    # Minor ticks exist for grid/scale but have NO labels.
    ax.yaxis.set_minor_locator(
        LogLocator(
            base=10,
            subs=np.arange(2, 10) * 0.1,
        )
    )
    ax.yaxis.set_minor_formatter(NullFormatter())

    ax.tick_params(
        axis="y",
        which="major",
        labelsize=8,
    )

    ax.tick_params(
        axis="y",
        which="minor",
        length=2,
    )


def add_value_labels(ax, bars):
    """Add compact labels above each visible bar."""

    for bar in bars:
        height = bar.get_height()

        if not np.isfinite(height) or height <= 0:
            continue

        ax.annotate(
            f"{height:.2f}",
            xy=(
                bar.get_x() + bar.get_width() / 2,
                height,
            ),
            xytext=(0, 3),
            textcoords="offset points",
            ha="center",
            va="bottom",
            fontsize=7,
        )


def save_all_formats(fig, out_dir, filename_stem):
    fig.savefig(
        out_dir / f"{filename_stem}.pdf",
        bbox_inches="tight",
    )


# ----------------------------------------------------------------------
# Operation plots
# ----------------------------------------------------------------------

def plot_operation(operation, entries, out_dir):
    subset = [
        e
        for e in entries
        if e["full_load"] in REPRESENTATIVE_FULL_LOAD
    ]

    baseline_time = get_baseline_time(
        subset,
        REPRESENTATIVE_FULL_LOAD[0],
    )

    if baseline_time is None:
        print(f"  [skip] {operation}: no baseline entry found")
        return

    other_systems = order_systems(
        {
            e["system"]
            for e in subset
            if e["system"] != BASELINE_SYSTEM
        }
    )

    systems = [BASELINE_SYSTEM] + other_systems

    if not other_systems:
        print(
            f"  [skip] {operation}: "
            f"no non-baseline entries found"
        )
        return

    speedups = [1.0]

    for system in other_systems:
        matches = [
            e
            for e in subset
            if e["system"] == system
        ]

        if matches:
            speedups.append(
                matches[0]["time"] / baseline_time
            )
        else:
            speedups.append(np.nan)

    # --------------------------------------------------------------
    # Figure sizing
    #
    # Do NOT make the width proportional to the number of bars.
    # One bar = one category here, so a small fixed amount per system
    # is enough.
    # --------------------------------------------------------------

    fig_width = max(
        OPERATION_MIN_FIG_WIDTH,
        OPERATION_WIDTH_PER_SYSTEM * len(systems)
        + OPERATION_EXTRA_WIDTH,
    )

    # DEBUG
    print("   plot", operation, systems, [round(s, 3) for s in speedups])

    fig, ax = plt.subplots(
        figsize=(fig_width, OPERATION_FIG_HEIGHT)
    )

    x = np.arange(len(systems))

    # Each category gets one bar, so use most of the available slot.
    bar_width = min(0.65, GROUP_WIDTH)

    bars = ax.bar(
        x,
        speedups,
        width=bar_width,
        color=[
            system_color(s, i)
            for i, s in enumerate(systems)
        ],
    )

    add_value_labels(ax, bars)

    ax.set_yscale("log")

    configure_log_y_axis(
        ax,
        speedups,
    )

    ax.axhline(
        1.0,
        color="gray",
        linestyle="--",
        linewidth=0.7,
        zorder=0,
    )

    ax.set_xticks(x)
    ax.set_xticklabels(
        systems,
        rotation=20,
        ha="right",
        fontsize=8,
    )

    ax.set_title(
        operation,
        fontsize=10,
    )

    ax.margins(x=0.04)

    fig.tight_layout()

    filename = operation.lower()

    save_all_formats(
        fig,
        out_dir,
        filename,
    )

    plt.close(fig)

    print(
        f"  [ok]   {operation} -> "
        f"{out_dir / (filename + '.pdf')}"
    )


# ----------------------------------------------------------------------
# Load plot
# ----------------------------------------------------------------------

def plot_load(entries, out_dir):
    subset = [
        e
        for e in entries
        if e["operation"] == "Load"
    ]

    if not subset:
        print("  [skip] Load: no entries found")
        return

    full_load_values = sorted(
        {
            e["full_load"]
            for e in subset
        }
    )

    other_systems = order_systems(
        {
            e["system"]
            for e in subset
            if e["system"] != BASELINE_SYSTEM
        }
    )

    systems = [
        BASELINE_SYSTEM
    ] + other_systems

    if not other_systems:
        print(
            "  [skip] Load: "
            "no non-baseline entries found"
        )
        return

    n_groups = len(full_load_values)
    n_systems = len(systems)


    fig_width = max(
        3.8,
        1.0 * n_groups + 1.5,
    )

    fig, ax = plt.subplots(
        figsize=(fig_width, LOAD_FIG_HEIGHT)
    )

    x = np.arange(n_groups)

    # Total width occupied by all bars in one group.
    total_group_width = GROUP_WIDTH

    # Bars are packed tightly inside the group.
    bar_width = total_group_width / n_systems

    all_speedups = []

    for i, system in enumerate(systems):

        speedups = []

        for fl in full_load_values:
            baseline_time = get_baseline_time(
                subset,
                fl,
            )

            if system == BASELINE_SYSTEM:
                has_data = any(
                    e["system"] == system
                    and e["full_load"] == fl
                    for e in subset
                )

                speedups.append(
                    1.0 if has_data else np.nan
                )

                continue

            matches = [
                e
                for e in subset
                if e["system"] == system
                and e["full_load"] == fl
            ]

            if (
                baseline_time is not None
                and matches
            ):
                speedups.append(
                    matches[0]["time"] /baseline_time
                )
            else:
                speedups.append(np.nan)

        all_speedups.extend(
            v
            for v in speedups
            if np.isfinite(v)
            and v > 0
        )

        # Center all system bars around the group center.
        offset = (
            i - (n_systems - 1) / 2
        ) * bar_width

        bars = ax.bar(
            x + offset,
            speedups,
            width=bar_width * 0.92,
            label=system,
            color=system_color(system, i),
        )

        add_value_labels(
            ax,
            bars,
        )

    ax.set_yscale("log")

    configure_log_y_axis(
        ax,
        all_speedups,
    )

    ax.axhline(
        1.0,
        color="gray",
        linestyle="--",
        linewidth=0.7,
        zorder=0,
    )

    ax.set_xticks(x)

    ax.set_xticklabels(
        [
            FULL_LOAD_LABELS.get(
                value,
                str(value),
            )
            for value in full_load_values
        ],
        fontsize=8,
    )

    ax.set_title(
        "Load",
        fontsize=10,
    )

    # Very small horizontal margins.
    ax.margins(x=0.04)

    # Legend below the plot, centered.
    ax.legend(
        loc="upper center",
        bbox_to_anchor=(0.5, -0.12),
        fontsize=7,
        ncol=min(n_systems, 3),
        frameon=False,
    )

    # Reserve space below the axes for the legend.
    fig.subplots_adjust(
        top=0.90,
        bottom=0.28,
        left=0.14,
        right=0.98,
    )

    save_all_formats(
        fig,
        out_dir,
        "load",
    )

    plt.close(fig)

    print(
        f"  [ok]   Load -> "
        f"{out_dir / 'load.pdf'}"
    )


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main():
    if len(sys.argv) < 2:
        print(
            f"Usage: {sys.argv[0]} "
            "<benchmark_results.json> [more.json ...]"
        )
        sys.exit(1)

    OUTPUT_DIR.mkdir(
        exist_ok=True
    )

    entries = list(
        load_entries(sys.argv[1:])
    )

    if not entries:
        print(
            "No entries parsed -- "
            "check the JSON files and naming scheme."
        )
        sys.exit(1)

    operations = sorted(
        {
            e["operation"]
            for e in entries
            if e["operation"] != "Load"
        }
    )

    print(
        f"Found {len(entries)} entries, "
        f"operations: {operations}, "
        f"systems: "
        f"{sorted({e['system'] for e in entries})}"
    )

    plot_load(
        entries,
        OUTPUT_DIR,
    )

    for operation in operations:
        plot_operation(
            operation,
            [
                e
                for e in entries
                if e["operation"] == operation
            ],
            OUTPUT_DIR,
        )
    
    for op in ("Subdivision", "Collapse"):
        print(op, sorted({(e["system"], e["full_load"]) for e in entries if e["operation"] == op}))


if __name__ == "__main__":
    main()
    
    