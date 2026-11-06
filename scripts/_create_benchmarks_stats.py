#!/usr/bin/env python3
import csv
import json
import re
import sys
from collections import defaultdict
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt

# ----------------------------------------------------------------------
# Config -- keep in sync with plot_benchmarks.py
# ----------------------------------------------------------------------

OUTPUT_DIR = Path("plots")

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

FULL_LOAD_LABELS = {
    0: "Core Only",
    2: "Core + VV-HE",
    3: "Core + VF",
    4: "Core + EF",
    1: "All",
    10: "VALIDATE",
}

# CV (%) above which a benchmark is flagged as noisy in the console output.
NOISY_CV_PERCENT = 5.0

# How many of the noisiest benchmarks to print.
TOP_N_NOISY = 15

plt.rcParams["font.family"] = "serif"
plt.rcParams["font.serif"] = ["cmr10", "Computer Modern Roman", "DejaVu Serif"]
plt.rcParams["axes.formatter.use_mathtext"] = True

# ----------------------------------------------------------------------
# Parsing
# ----------------------------------------------------------------------

NAME_RE = re.compile(
    r"^(?P<system>[^/]+)/(?P<version>[^/]+)/"
    r"(?P<operation>[^/]+)/(?P<args>.+)$"
)
FULL_LOAD_RE = re.compile(r"Full_Load:(\d+)")
V_RE = re.compile(r"\|V\|:([^/]+)")

TIME_UNIT_TO_MS = {"ns": 1e-6, "us": 1e-3, "ms": 1.0, "s": 1e3}


def clean_system_label(raw):
    return SYSTEM_LABELS.get(raw, raw.rstrip("_"))


def order_key(system):
    return (
        SYSTEM_ORDER.index(system) if system in SYSTEM_ORDER else len(SYSTEM_ORDER),
        system,
    )


def v_sort_key(v):
    try:
        return (0, float(v))
    except ValueError:
        return (1, v)


def collect_runs(json_paths):
    """Return {benchmark name: {"meta": ..., "times": [ms, ...]}}."""

    runs = {}

    for path in json_paths:
        try:
            with open(path) as f:
                data = json.load(f)
        except (json.JSONDecodeError, OSError) as exc:
            print(f"  [warn] could not read {path}: {exc}")
            continue

        for b in data.get("benchmarks", []):
            if b.get("run_type") == "aggregate" or "real_time" not in b:
                continue

            name = b.get("name", "")
            m = NAME_RE.match(name)
            if not m:
                continue

            fl_m = FULL_LOAD_RE.search(m.group("args"))
            if not fl_m:
                continue

            v_m = V_RE.search(m.group("args"))

            if name not in runs:
                runs[name] = {
                    "meta": {
                        "system": clean_system_label(m.group("system")),
                        "operation": m.group("operation"),
                        "full_load": int(fl_m.group(1)),
                        "V": v_m.group(1) if v_m else "",
                    },
                    "times": [],
                }

            scale = TIME_UNIT_TO_MS.get(b.get("time_unit", "ns"), 1e-6)
            runs[name]["times"].append(b["real_time"] * scale)

    return runs


# ----------------------------------------------------------------------
# Statistics
# ----------------------------------------------------------------------

def per_benchmark_stats(runs):
    rows = []

    for name, r in runs.items():
        t = np.asarray(r["times"], dtype=float)
        n = len(t)
        mean = float(np.mean(t))
        # Sample standard deviation; undefined for a single run.
        std = float(np.std(t, ddof=1)) if n > 1 else float("nan")
        cv = std / mean * 100 if n > 1 and mean > 0 else float("nan")
        rel_range = (t.max() - t.min()) / mean * 100 if mean > 0 else float("nan")

        rows.append({
            **r["meta"],
            "name": name,
            "n": n,
            "mean_ms": mean,
            "std_ms": std,
            "cv_percent": cv,
            "min_ms": float(t.min()),
            "max_ms": float(t.max()),
            "range_percent": float(rel_range),
        })

    rows.sort(key=lambda r: (
        r["operation"],
        r["full_load"],
        order_key(r["system"]),
        v_sort_key(r["V"]),
    ))
    return rows


def summary_stats(rows):
    """Aggregate per (system, operation, full_load) over all |V| values."""

    groups = defaultdict(list)
    for r in rows:
        groups[(r["system"], r["operation"], r["full_load"])].append(r)

    summary = []
    for (system, op, fl), items in groups.items():
        cvs = [r["cv_percent"] for r in items if np.isfinite(r["cv_percent"])]
        summary.append({
            "system": system,
            "operation": op,
            "full_load": fl,
            "n_V_values": len(items),
            "runs_per_V_min": min(r["n"] for r in items),
            "runs_per_V_max": max(r["n"] for r in items),
            "mean_cv_percent": float(np.mean(cvs)) if cvs else float("nan"),
            "max_cv_percent": float(np.max(cvs)) if cvs else float("nan"),
            "worst_V": max(
                (r for r in items if np.isfinite(r["cv_percent"])),
                key=lambda r: r["cv_percent"],
                default={"V": ""},
            )["V"],
        })

    summary.sort(key=lambda s: (s["operation"], s["full_load"], order_key(s["system"])))
    return summary


# ----------------------------------------------------------------------
# Output
# ----------------------------------------------------------------------

def write_csv(path, rows, columns):
    with open(path, "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=columns, extrasaction="ignore")
        w.writeheader()
        for r in rows:
            w.writerow({
                k: (f"{v:.6g}" if isinstance(v, float) else v)
                for k, v in r.items()
            })


def column_label(op, fl, multi_fl_ops):
    if op in multi_fl_ops:
        return f"{op}\n{FULL_LOAD_LABELS.get(fl, fl)}"
    return op


def plot_cv_heatmap(summary, out_path):
    """Systems as rows, operations as columns, cell = mean CV (%) over |V|.

    Load is split per Full_Load value (one column each); for the other
    operations all Full_Load values are merged into one column, as in the
    speedup plots.
    """

    if not summary:
        return

    multi_fl_ops = {"Load"}

    cells = defaultdict(list)
    for s in summary:
        if np.isfinite(s["mean_cv_percent"]):
            col = (s["operation"], s["full_load"] if s["operation"] in multi_fl_ops else None)
            cells[(s["system"], col)].append(s["mean_cv_percent"])

    fl_order = list(FULL_LOAD_LABELS.keys())
    cols = sorted(
        {c for (_, c) in cells},
        key=lambda c: (
            c[0] != "Load",
            c[0],
            fl_order.index(c[1]) if c[1] in fl_order else len(fl_order),
        ),
    )
    systems = sorted({sys_ for (sys_, _) in cells}, key=order_key)

    if not cols or not systems:
        print("  [skip] heatmap: no CV values (need >= 2 runs per benchmark)")
        return

    grid = np.full((len(systems), len(cols)), np.nan)
    for i, s in enumerate(systems):
        for j, c in enumerate(cols):
            if (s, c) in cells:
                grid[i, j] = float(np.mean(cells[(s, c)]))

    fig_w = max(4.0, 0.75 * len(cols) + 2.2)
    fig_h = max(2.4, 0.35 * len(systems) + 1.4)
    fig, ax = plt.subplots(figsize=(fig_w, fig_h))

    vmax = np.nanmax(grid) if np.isfinite(grid).any() else 1.0
    im = ax.imshow(
        np.ma.masked_invalid(grid),
        cmap="Blues",
        vmin=0,
        vmax=max(vmax, 1e-9),
        aspect="auto",
    )

    for i in range(len(systems)):
        for j in range(len(cols)):
            v = grid[i, j]
            if not np.isfinite(v):
                ax.text(j, i, "–", ha="center", va="center", fontsize=7, color="#888888")
                continue
            ax.text(
                j, i, f"{v:.1f}",
                ha="center", va="center", fontsize=7,
                color="white" if v > 0.6 * vmax else "black",
            )

    ax.set_xticks(range(len(cols)))
    ax.set_xticklabels(
        [column_label(op, fl, multi_fl_ops) for op, fl in cols],
        rotation=35, ha="right", fontsize=7,
    )
    ax.set_yticks(range(len(systems)))
    ax.set_yticklabels(systems, fontsize=8)
    ax.set_title(r"Run-to-run variation (mean CV in %, over all $|V|$)", fontsize=9)

    cbar = fig.colorbar(im, ax=ax, fraction=0.04, pad=0.02)
    cbar.ax.tick_params(labelsize=7)
    cbar.set_label("CV (%)", fontsize=8)

    fig.tight_layout()
    fig.savefig(out_path, bbox_inches="tight")
    plt.close(fig)
    print(f"  [ok]   heatmap -> {out_path}")


def print_report(rows, summary):
    with_cv = [r for r in rows if np.isfinite(r["cv_percent"])]
    single = len(rows) - len(with_cv)

    print()
    print(f"Benchmarks: {len(rows)}  (with >= 2 runs: {len(with_cv)}, single run: {single})")

    if not with_cv:
        print("Not enough runs to compute a standard deviation.")
        return

    cvs = np.array([r["cv_percent"] for r in with_cv])
    print(
        f"CV over all benchmarks:  median {np.median(cvs):.2f} %   "
        f"mean {np.mean(cvs):.2f} %   max {np.max(cvs):.2f} %"
    )
    n_noisy = int(np.sum(cvs > NOISY_CV_PERCENT))
    print(f"Benchmarks with CV > {NOISY_CV_PERCENT:g} %: {n_noisy}")

    print()
    print(f"Noisiest {min(TOP_N_NOISY, len(with_cv))} benchmarks:")
    print(f"  {'CV %':>7}  {'n':>3}  {'mean ms':>11}  {'std ms':>10}  name")
    for r in sorted(with_cv, key=lambda r: r["cv_percent"], reverse=True)[:TOP_N_NOISY]:
        print(
            f"  {r['cv_percent']:7.2f}  {r['n']:3d}  "
            f"{r['mean_ms']:11.4g}  {r['std_ms']:10.4g}  {r['name']}"
        )


# ----------------------------------------------------------------------
# Main
# ----------------------------------------------------------------------

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} <folder_with_benchmark_jsons>")
        sys.exit(1)

    folder = Path(sys.argv[1])
    if not folder.is_dir():
        print(f"Not a directory: {folder}")
        sys.exit(1)

    json_paths = sorted(p for p in folder.rglob("*.json") if p.is_file())
    if not json_paths:
        print(f"No .json files found in {folder}")
        sys.exit(1)

    print(f"Reading {len(json_paths)} JSON file(s) from {folder}")

    runs = collect_runs(json_paths)
    if not runs:
        print("No entries parsed -- check the JSON files and naming scheme.")
        sys.exit(1)

    OUTPUT_DIR.mkdir(exist_ok=True)

    rows = per_benchmark_stats(runs)
    summary = summary_stats(rows)

    per_bench_path = OUTPUT_DIR / "stats_per_benchmark.csv"
    write_csv(per_bench_path, rows, [
        "system", "operation", "full_load", "V", "n",
        "mean_ms", "std_ms", "cv_percent", "min_ms", "max_ms",
        "range_percent", "name",
    ])
    print(f"  [ok]   per-benchmark stats -> {per_bench_path}")

    summary_path = OUTPUT_DIR / "stats_summary.csv"
    write_csv(summary_path, summary, [
        "system", "operation", "full_load", "n_V_values",
        "runs_per_V_min", "runs_per_V_max",
        "mean_cv_percent", "max_cv_percent", "worst_V",
    ])
    print(f"  [ok]   summary stats       -> {summary_path}")

    plot_cv_heatmap(summary, OUTPUT_DIR / "stats_cv_heatmap.pdf")

    print_report(rows, summary)


if __name__ == "__main__":
    main()
