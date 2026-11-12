#!/usr/bin/env python3
import json
import re
import sys
from collections import defaultdict
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
    "D_Ragged____": "PM-Reuse",
    "Apd_Ragged__": "PM-Append",
    "VecVec______": "PM-VecVec",
    "Vec_Eigen___": "PM-VecEigen",
    "Apd_Ragged_V": "PM-Validate",
}
SYSTEM_ORDER = list(SYSTEM_LABELS.values())

# Full_Load numeric value -> display label for the Load benchmark's x-axis.
FULL_LOAD_LABELS = {
    0: "Core Only",
    2: "Core + VV-HE",
    3: "Core + VF",
    4: "Core + EF",
    1: "All",
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
    "PM-Reuse":         "#1baf7a",   # aqua
    "PM-Append":        "#eda100",   # yellow
    "PM-VecVec":        "#e87ba4",   # magenta
    "PM-VecEigen":      "#008300",   # green
    "PM-Validate":      "#6250d6",   # violet
}

DEFAULT_COLOR_CYCLE = plt.rcParams["axes.prop_cycle"].by_key()["color"]

# ----------------------------------------------------------------------
# Machines (test systems)
# ----------------------------------------------------------------------
# Folders are passed on the command line, one per machine:
#   python plot_benchmarks.py results_thinkpad results/results_windows results/results_apple
# or with explicit names:
#   python plot_benchmarks.py ThinkPad=results_thinkpad Windows=results/results_windows ...
# Without a name, the folder name is used with this prefix stripped.
MACHINE_PREFIX_STRIP = "results_"

# True: label machines "System 1", "System 2", ... in command-line order
# (the mapping folder -> number is printed when the script runs).
# False: use the names below / Name=path.
USE_SYSTEM_NUMBERS = True

# Optional nicer display names for machines (key = derived/explicit name).
MACHINE_LABELS = {
    "thinkpad": "ThinkPad",
    "windows": "Windows",
    "apple": "Apple",
}

# Bar style per machine, in command-line order. Same library color,
# different fill pattern. hatch=None -> solid bar.
MACHINE_STYLES = [
    {"hatch": None},
    {"hatch": "////"},
    {"hatch": "..."},
    {"hatch": "xx"},
    {"hatch": "\\\\\\\\"},
]

# Color of the hatch lines/dots drawn on top of the library color.
HATCH_COLOR = "white"
plt.rcParams["hatch.linewidth"] = 0.8

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
# Larger than before because the value labels are now vertical.
Y_TOP_MARGIN = 2.2

# Value labels above bars: rotation (90 = vertical) and font size.
VALUE_LABEL_ROTATION = 90
VALUE_LABEL_FONTSIZE = 6

# How much room to leave below the smallest bar.
Y_BOTTOM_MARGIN = 1.25



# Figure sizing
LOAD_FIG_HEIGHT = 3.1
LOAD_GROUP_WIDTH = 0.88
# Width per Full_Load group; multiplied by the number of machines / 3.
LOAD_WIDTH_PER_GROUP = 2.0

# Fraction of each system's slot filled by its (touching) machine bars.
# The rest is the gap between systems.
SYSTEM_FILL = 0.88

OPERATION_FIG_HEIGHT = 2.8

# Width per system for non-Load plots (for 3 machines; scales with count).
# Smaller = narrower graphs.
OPERATION_WIDTH_PER_SYSTEM = 0.7

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

# Matches the "|V|:<value>/" segment so it can be stripped from the name.
# Benchmarks that differ only in |V| then share the same key and get averaged.
V_SEGMENT_RE = re.compile(r"\|V\|:[^/]*/?")
V_VALUE_RE = re.compile(r"\|V\|:([^/]+)")

# How the per-|V| ratios are combined into one bar value.
# Every |V| gets the same weight either way.
#   "geometric"  -- geometric mean (standard for ratios / speedups)
#   "arithmetic" -- plain mean of the ratios
RATIO_MEAN = "geometric"

# Google Benchmark time units -> nanoseconds, so averaging is unit-safe.
TIME_UNIT_TO_NS = {
    "ns": 1.0,
    "us": 1e3,
    "ms": 1e6,
    "s": 1e9,
}


def clean_system_label(raw: str) -> str:
    return SYSTEM_LABELS.get(raw, raw.rstrip("_"))


def find_json_files(folder: Path):
    return sorted(p for p in folder.rglob("*.json") if p.is_file())


def load_entries(json_paths, machine):
    """Return one {system, operation, full_load, times_by_v} entry per
    benchmark name with the |V| segment removed.

    times_by_v maps each |V| value to the mean time of that exact benchmark
    over all JSON files. The |V| values are kept apart here so that each
    mesh size can later be compared to the baseline at the same size and
    then weighted equally (see compute_speedup).

    Aggregate entries (mean/median/stddev) are skipped; only individual
    runs are averaged. Times are normalized to nanoseconds.
    """

    times = defaultdict(lambda: defaultdict(list))
    meta = {}

    for path in json_paths:
        try:
            with open(path) as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  [warn] could not read {path}: {exc}")
            continue

        for b in data.get("benchmarks", []):
            if b.get("run_type") == "aggregate":
                continue

            name = b.get("name", "")
            m = NAME_RE.match(name)
            if not m:
                continue

            fl_m = FULL_LOAD_RE.search(m.group("args"))
            if not fl_m:
                continue

            if "real_time" not in b:
                continue

            scale = TIME_UNIT_TO_NS.get(b.get("time_unit", "ns"), 1.0)

            key = V_SEGMENT_RE.sub("", name)
            v_m = V_VALUE_RE.search(name)
            v = v_m.group(1) if v_m else ""
            times[key][v].append(b["real_time"] * scale)

            if key not in meta:
                meta[key] = {
                    "system": clean_system_label(m.group("system")),
                    "operation": m.group("operation"),
                    "full_load": int(fl_m.group(1)),
                }

    entries = []
    for key, by_v in times.items():
        entry = dict(meta[key])
        entry["machine"] = machine
        entry["times_by_v"] = {
            v: float(np.mean(vals)) for v, vals in by_v.items()
        }
        entry["n_samples"] = sum(len(vals) for vals in by_v.values())
        entries.append(entry)

    return entries


def compute_speedup(entry, baseline_entry):
    """Time ratio entry / baseline, with every |V| weighted equally.

    For each |V| present in both entries the ratio is computed at that
    mesh size; the ratios are then combined with RATIO_MEAN. Sizes that
    only one of the two has are ignored (and reported once).
    """

    if entry is None or baseline_entry is None:
        return np.nan

    a = entry["times_by_v"]
    b = baseline_entry["times_by_v"]
    common = sorted(set(a) & set(b))

    missing = set(a) ^ set(b)
    if missing:
        warn_key = (
            entry.get("machine"), entry["system"], entry["operation"],
            entry["full_load"], baseline_entry["full_load"],
        )
        if warn_key not in _warned_missing_v:
            _warned_missing_v.add(warn_key)
            print(
                f"  [warn] [{entry.get('machine')}] "
                f"{entry['system']} / {entry['operation']} "
                f"(Full_Load {entry['full_load']}): |V| values "
                f"{sorted(missing)} not present for both it and the "
                f"baseline -- ignored"
            )

    ratios = [
        a[v] / b[v]
        for v in common
        if b[v] > 0 and a[v] > 0
    ]

    if not ratios:
        return np.nan

    if RATIO_MEAN == "arithmetic":
        return float(np.mean(ratios))

    return float(np.exp(np.mean(np.log(ratios))))


_warned_missing_v = set()


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


def get_baseline_entry(entries, full_load, machine=None):
    """Baseline entry for the given Full_Load value (on the given machine).

    Falls back to whichever single value the baseline system actually has.
    """

    candidates = [
        e for e in entries
        if e["system"] == BASELINE_SYSTEM
        and (machine is None or e.get("machine") == machine)
    ]

    exact = [
        e for e in candidates
        if e["full_load"] == full_load
    ]

    if exact:
        return exact[0]

    return candidates[0] if candidates else None


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
            xytext=(0, 2),
            textcoords="offset points",
            ha="center",
            va="bottom",
            rotation=VALUE_LABEL_ROTATION,
            fontsize=VALUE_LABEL_FONTSIZE,
        )


def machine_style(j):
    return MACHINE_STYLES[j % len(MACHINE_STYLES)]


def draw_machine_bars(ax, xs, heights, width, colors, j, label=None):
    """Bars for machine j: library color fill + that machine's pattern."""

    hatch = machine_style(j)["hatch"]
    return ax.bar(
        xs,
        heights,
        width=width,
        color=colors,
        hatch=hatch,
        edgecolor=HATCH_COLOR if hatch else "none",
        linewidth=0,
        label=label,
    )


def machine_legend_handles(machines):
    """Gray patches showing each machine's pattern (color-independent)."""
    from matplotlib.patches import Patch

    return [
        Patch(
            facecolor="#7a7a7a",
            hatch=machine_style(j)["hatch"],
            edgecolor=HATCH_COLOR if machine_style(j)["hatch"] else "none",
            linewidth=0,
            label=m,
        )
        for j, m in enumerate(machines)
    ]


def machine_bar_offsets(n_machines, slot_width):
    """Offsets (relative to the slot center) and width of touching bars."""

    w = slot_width * SYSTEM_FILL / n_machines
    offsets = [(j - (n_machines - 1) / 2) * w for j in range(n_machines)]
    return offsets, w


def layout_slots(systems, n_machines, total_width):
    """Slot centers (relative to the group center, summing to total_width),
    the normal slot width and the single-bar width.

    The baseline system gets a narrow slot holding a single bar; every
    other system gets a full slot with n_machines touching bars.
    """

    gap_frac = 1 - SYSTEM_FILL
    bar_frac = SYSTEM_FILL / n_machines
    units = [
        (bar_frac + gap_frac) if s == BASELINE_SYSTEM else 1.0
        for s in systems
    ]
    slot = total_width / sum(units)
    widths = [u * slot for u in units]

    centers, pos = [], -total_width / 2
    for w in widths:
        centers.append(pos + w / 2)
        pos += w
    return np.array(centers), slot, slot * bar_frac


def save_all_formats(fig, out_dir, filename_stem):
    fig.savefig(
        out_dir / f"{filename_stem}.pdf",
        bbox_inches="tight",
    )


def all_v_values(entries):
    """Every |V| value in the data, sorted numerically where possible."""

    values = {v for e in entries for v in e["times_by_v"] if v != ""}

    def key(v):
        try:
            return (0, float(v))
        except ValueError:
            return (1, v)

    return sorted(values, key=key)


def entries_for_v(entries, v):
    """Copy of the entries restricted to a single |V| value.

    Entries that have no run for this |V| are dropped.
    """

    out = []
    for e in entries:
        if v in e["times_by_v"]:
            e2 = dict(e)
            e2["times_by_v"] = {v: e["times_by_v"][v]}
            out.append(e2)
    return out


def title_suffix(v):
    # Mathtext, because the serif font has no "|" glyph.
    return "" if v is None else rf" ($|V|$ = {v})"


def file_suffix(v):
    if v is None:
        return ""
    safe = re.sub(r"[^A-Za-z0-9.\-]+", "-", str(v))
    return f"_{safe}"


# ----------------------------------------------------------------------
# Operation plots
# ----------------------------------------------------------------------

def plot_operation(operation, entries, out_dir, machines, v=None):
    """v=None: averaged over all |V| (file <operation>.pdf).
    v=<value>: only that |V| (file <operation>_<value>.pdf).

    One slot per system; inside it one touching bar per machine, each
    relative to the baseline on that same machine."""

    if v is not None:
        entries = entries_for_v(entries, v)

    label = operation + ("" if v is None else f" |V|={v}")

    subset = [
        e
        for e in entries
        if e["full_load"] in REPRESENTATIVE_FULL_LOAD
    ]

    if not any(e["system"] == BASELINE_SYSTEM for e in subset):
        print(f"  [skip] {label}: no baseline entry found")
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
            f"  [skip] {label}: "
            f"no non-baseline entries found"
        )
        return

    n_machines = len(machines)

    # speedups[j][i] = machine j, system i
    speedups = []
    for machine in machines:
        m_subset = [e for e in subset if e.get("machine") == machine]
        baseline_entry = get_baseline_entry(
            m_subset,
            REPRESENTATIVE_FULL_LOAD[0],
        )
        row = []
        for system in systems:
            if baseline_entry is None:
                row.append(np.nan)
                continue
            if system == BASELINE_SYSTEM:
                row.append(1.0)
                continue
            matches = [e for e in m_subset if e["system"] == system]
            row.append(
                compute_speedup(matches[0], baseline_entry)
                if matches else np.nan
            )
        speedups.append(row)

    # Width in "slot" units: full slots for libraries, a narrow one for
    # the baseline.
    total_width = (len(systems) - 1) + (1 - SYSTEM_FILL) + SYSTEM_FILL / n_machines

    fig_width = max(
        OPERATION_MIN_FIG_WIDTH,
        OPERATION_WIDTH_PER_SYSTEM * (n_machines / 3) * total_width
        + OPERATION_EXTRA_WIDTH,
    )

    # DEBUG
    for machine, row in zip(machines, speedups):
        print("   plot", label, machine, systems, [round(s, 3) for s in row])

    fig, ax = plt.subplots(
        figsize=(fig_width, OPERATION_FIG_HEIGHT)
    )

    colors = [system_color(s, i) for i, s in enumerate(systems)]

    # Baseline (index 0) gets one narrow bar; others get n_machines bars.
    x, slot, bar_w = layout_slots(systems, n_machines, total_width)
    offsets, _ = machine_bar_offsets(n_machines, slot)

    baseline_has_data = any(np.isfinite(row[0]) for row in speedups)
    bars = ax.bar(
        x[0], 1.0 if baseline_has_data else np.nan,
        width=bar_w, color=colors[0], linewidth=0,
    )
    add_value_labels(ax, bars)

    for j, machine in enumerate(machines):
        bars = draw_machine_bars(
            ax, x[1:] + offsets[j], speedups[j][1:], bar_w, colors[1:], j,
        )
        add_value_labels(ax, bars)

    ax.set_yscale("log")

    configure_log_y_axis(
        ax,
        [s for row in speedups for s in row],
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
        operation + title_suffix(v),
        fontsize=10,
    )

    ax.margins(x=0.02)

    # Machine pattern legend, top-left inside the axes.
    ax.legend(
        handles=machine_legend_handles(machines),
        loc="upper left",
        fontsize=6,
        frameon=False,
        ncol=n_machines,
        handlelength=1.6,
        columnspacing=0.8,
    )

    fig.tight_layout()

    filename = operation.lower() + file_suffix(v)

    save_all_formats(
        fig,
        out_dir,
        filename,
    )

    plt.close(fig)

    print(
        f"  [ok]   {label} -> "
        f"{out_dir / (filename + '.pdf')}"
    )


# ----------------------------------------------------------------------
# Load plot
# ----------------------------------------------------------------------

def plot_load(entries, out_dir, machines, v=None):
    """v=None: averaged over all |V| (file load.pdf).
    v=<value>: only that |V| (file load_<value>.pdf)."""
    from matplotlib.patches import Patch

    if v is not None:
        entries = entries_for_v(entries, v)

    label = "Load" + ("" if v is None else f" |V|={v}")

    subset = [
        e
        for e in entries
        if e["operation"] == "Load"
    ]

    if not subset:
        print(f"  [skip] {label}: no entries found")
        return

    FULL_LOAD_ORDER = list(FULL_LOAD_LABELS.keys())

    full_load_values = sorted(
        {
            e["full_load"]
            for e in subset
        },
        key=lambda v: (
            FULL_LOAD_ORDER.index(v) if v in FULL_LOAD_ORDER else len(FULL_LOAD_ORDER),
            v,
        ),
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
            f"  [skip] {label}: "
            "no non-baseline entries found"
        )
        return

    n_groups = len(full_load_values)
    n_systems = len(systems)
    n_machines = len(machines)

    fig_width = max(
        3.8,
        LOAD_WIDTH_PER_GROUP * (n_machines / 3) * n_groups + 1.5,
    )

    fig, ax = plt.subplots(
        figsize=(fig_width, LOAD_FIG_HEIGHT)
    )

    x = np.arange(n_groups)

    # Each group is split into one slot per system; each slot holds
    # n_machines touching bars.
    # The baseline gets one narrow slot with a single bar.
    centers, slot_width, bar_w = layout_slots(
        systems, n_machines, LOAD_GROUP_WIDTH,
    )
    m_offsets, _ = machine_bar_offsets(n_machines, slot_width)

    all_speedups = []

    for i, system in enumerate(systems):
        color = system_color(system, i)
        sys_offset = centers[i]

        if system == BASELINE_SYSTEM:
            speedups = [
                1.0 if any(
                    e["system"] == system and e["full_load"] == fl
                    for e in subset
                ) else np.nan
                for fl in full_load_values
            ]
            all_speedups.extend(s for s in speedups if np.isfinite(s))
            bars = ax.bar(
                x + sys_offset, speedups, width=bar_w,
                color=color, linewidth=0,
            )
            add_value_labels(ax, bars)
            continue

        for j, machine in enumerate(machines):
            m_subset = [e for e in subset if e.get("machine") == machine]
            speedups = []

            for fl in full_load_values:
                baseline_entry = get_baseline_entry(m_subset, fl)

                if system == BASELINE_SYSTEM:
                    has_data = any(
                        e["system"] == system and e["full_load"] == fl
                        for e in m_subset
                    )
                    speedups.append(1.0 if has_data else np.nan)
                    continue

                matches = [
                    e
                    for e in m_subset
                    if e["system"] == system
                    and e["full_load"] == fl
                ]

                if baseline_entry is not None and matches:
                    speedups.append(
                        compute_speedup(matches[0], baseline_entry)
                    )
                else:
                    speedups.append(np.nan)

            all_speedups.extend(
                s for s in speedups if np.isfinite(s) and s > 0
            )

            bars = draw_machine_bars(
                ax,
                x + sys_offset + m_offsets[j],
                speedups,
                bar_w,
                color,
                j,
            )

            add_value_labels(ax, bars)

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
        "Load" + title_suffix(v),
        fontsize=10,
    )

    ax.margins(x=0.02)

    # Legend below the plot: library colors, then machine patterns.
    system_handles = [
        Patch(facecolor=system_color(s, i), label=s)
        for i, s in enumerate(systems)
    ]
    leg1 = ax.legend(
        handles=system_handles,
        loc="upper center",
        bbox_to_anchor=(0.5, -0.12),
        fontsize=7,
        ncol=min(n_systems, 4),
        frameon=False,
    )
    ax.add_artist(leg1)
    ax.legend(
        handles=machine_legend_handles(machines),
        loc="upper left",
        fontsize=6,
        frameon=False,
        ncol=n_machines,
        handlelength=1.6,
        columnspacing=0.8,
    )

    # Reserve space below the axes for the legend.
    fig.subplots_adjust(
        top=0.90,
        bottom=0.30,
        left=0.08,
        right=0.99,
    )

    filename = "load" + file_suffix(v)

    save_all_formats(
        fig,
        out_dir,
        filename,
    )

    plt.close(fig)

    print(
        f"  [ok]   {label} -> "
        f"{out_dir / (filename + '.pdf')}"
    )


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def parse_machine_arg(arg):
    """'Name=path' or just 'path' -> (display name, Path)."""

    if "=" in arg:
        name, path = arg.split("=", 1)
    else:
        path = arg
        name = Path(arg.rstrip("/\\")).name
        if MACHINE_PREFIX_STRIP and name.startswith(MACHINE_PREFIX_STRIP):
            name = name[len(MACHINE_PREFIX_STRIP):]
    return MACHINE_LABELS.get(name, name), Path(path)


def main():
    if len(sys.argv) < 2:
        print(
            f"Usage: {sys.argv[0]} <machine_folder> [<machine_folder> ...]\n"
            f"       {sys.argv[0]} Name=<folder> Name=<folder> ...\n"
            f"Example: {sys.argv[0]} results_thinkpad "
            f"results/results_windows results/results_apple"
        )
        sys.exit(1)

    machines = []
    entries = []

    for arg in sys.argv[1:]:
        name, folder = parse_machine_arg(arg)

        if not folder.is_dir():
            print(f"Not a directory: {folder}")
            sys.exit(1)

        json_paths = find_json_files(folder)

        if not json_paths:
            print(f"No .json files found in {folder} -- skipping {name}")
            continue

        if USE_SYSTEM_NUMBERS:
            name = f"System {len(machines) + 1}"
        print(f"[{name}] reading {len(json_paths)} JSON file(s) from {folder}")
        machines.append(name)
        entries.extend(load_entries(json_paths, name))

    if not entries:
        print(
            "No entries parsed -- "
            "check the JSON files and naming scheme."
        )
        sys.exit(1)

    OUTPUT_DIR.mkdir(
        exist_ok=True
    )

    operations = sorted(
        {
            e["operation"]
            for e in entries
            if e["operation"] != "Load"
        }
    )

    print(
        f"Found {len(entries)} averaged entries "
        f"(from {sum(e['n_samples'] for e in entries)} runs), "
        f"machines: {machines}, "
        f"operations: {operations}, "
        f"systems: "
        f"{sorted({e['system'] for e in entries})}"
    )

    v_values = all_v_values(entries)
    print(f"|V| values: {v_values}")

    # None = averaged over all |V| (plain filenames),
    # then one set of plots per individual |V| (filenames with _<|V|>).
    for v in [None] + v_values:
        if v is not None:
            print(f"\n--- |V| = {v} ---")

        plot_load(
            entries,
            OUTPUT_DIR,
            machines,
            v=v,
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
                machines,
                v=v,
            )

    for op in ("Subdivision", "Collapse"):
        print(op, sorted({(e["machine"], e["system"], e["full_load"]) for e in entries if e["operation"] == op}))


if __name__ == "__main__":
    main()
    
    
    
    