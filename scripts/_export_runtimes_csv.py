#!/usr/bin/env python3
"""
Export runtimes relative to OpenMesh (same numbers as the plots) to a CSV.

    python export_relative_csv.py results_thinkpad results/results_windows results/results_apple

Output (relative_runtimes.csv):

    benchmark,library,sys1,sys2,sys3
    Load--0,om,1.000,1.000,1.000
    Load--0,pmp,0.553,0.101,1.012
    ...

Values are runtime / OpenMesh runtime on the same system
(< 1 = faster than OpenMesh), averaged over all |V| exactly like the plots.

Also writes raw_runtimes.csv with absolute mean runtimes (in RAW_UNIT),
one row per benchmark, |V| and library:

    benchmark,V,library,sys1_ms,sys2_ms,sys3_ms
    Load--0,1000,om,1.0123,0.6051,0.8034

Must live next to plot_benchmarks.py (it reuses its parsing code).
"""
import csv
import sys

import _plot_benchmarks_compare as pb

OUTPUT_CSV = "results/relative_runtimes.csv"
DECIMALS = 3

# Raw runtimes: one row per |V| (averaging absolute times over different
# mesh sizes would be dominated by the largest mesh).
RAW_OUTPUT_CSV = "results/raw_runtimes.csv"
RAW_UNIT = "ms"          # "ns", "us", "ms" or "s"
RAW_DECIMALS = 4

# Display name in plot_benchmarks -> short CSV name.
SHORT_NAMES = {
    "OpenMesh": "om",
    "SurfaceMesh": "pmp",
    "Libigl": "libigl",
    "PM-Reuse": "pm-reuse",
    "PM-Append": "pm-append",
    "PM-VecVec": "pm-vecvec",
    "PM-VecEigen": "pm-veceigen",
    "PM-Validate": "pm-validate",
}

# Operation order in the CSV (others follow alphabetically).
OPERATION_ORDER = ["Load"]


def baseline_for(entries, operation, full_load):
    """OpenMesh entry for this operation/Full_Load on one machine.
    Falls back to Full_Load 1 ("All"), then to anything available."""
    om = [e for e in entries
          if e["system"] == pb.BASELINE_SYSTEM and e["operation"] == operation]
    for fl in (full_load, 1):
        hit = [e for e in om if e["full_load"] == fl]
        if hit:
            return hit[0]
    return om[0] if om else None


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <sys1_folder> <sys2_folder> <sys3_folder> ...")
        sys.exit(1)

    machines, by_machine = [], {}
    for k, arg in enumerate(sys.argv[1:], start=1):
        _, folder = pb.parse_machine_arg(arg)
        paths = pb.find_json_files(folder)
        if not paths:
            print(f"No .json files in {folder} -- skipping")
            continue
        name = f"sys{k}"
        print(f"{name} = {folder} ({len(paths)} files)")
        machines.append(name)
        by_machine[name] = pb.load_entries(paths, name)

    all_entries = [e for es in by_machine.values() for e in es]
    if not all_entries:
        print("No entries parsed.")
        sys.exit(1)

    ops = sorted({e["operation"] for e in all_entries},
                 key=lambda o: (OPERATION_ORDER.index(o)
                                if o in OPERATION_ORDER else len(OPERATION_ORDER), o))
    fl_order = list(pb.FULL_LOAD_LABELS.keys())
    libraries = pb.order_systems({e["system"] for e in all_entries})
    libraries = [pb.BASELINE_SYSTEM] + [s for s in libraries if s != pb.BASELINE_SYSTEM]

    rows = []
    for op in ops:
        fls = sorted({e["full_load"] for e in all_entries if e["operation"] == op},
                     key=lambda f: (fl_order.index(f) if f in fl_order else len(fl_order), f))
        for fl in fls:
            for lib in libraries:
                values = []
                for m in machines:
                    es = by_machine[m]
                    base = baseline_for(es, op, fl)
                    match = [e for e in es if e["operation"] == op
                             and e["full_load"] == fl and e["system"] == lib]
                    if lib == pb.BASELINE_SYSTEM:
                        val = 1.0 if match else None
                    elif base and match:
                        val = pb.compute_speedup(match[0], base)
                    else:
                        val = None
                    values.append(
                        "" if val is None or val != val else f"{val:.{DECIMALS}f}"
                    )
                if any(values):
                    rows.append([f"{op}--{fl}", SHORT_NAMES.get(lib, lib.lower())] + values)

    with open(OUTPUT_CSV, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["benchmark", "library"] + machines)
        w.writerows(rows)

    print(f"Wrote {len(rows)} rows to {OUTPUT_CSV}")

    # ---------------- raw runtimes ----------------
    unit_div = pb.TIME_UNIT_TO_NS[RAW_UNIT]
    v_values = pb.all_v_values(all_entries) or [""]

    raw_rows = []
    for op in ops:
        fls = sorted({e["full_load"] for e in all_entries if e["operation"] == op},
                     key=lambda f: (fl_order.index(f) if f in fl_order else len(fl_order), f))
        for fl in fls:
            for v in v_values:
                for lib in libraries:
                    values = []
                    for m in machines:
                        match = [e for e in by_machine[m]
                                 if e["operation"] == op and e["full_load"] == fl
                                 and e["system"] == lib and v in e["times_by_v"]]
                        values.append(
                            f"{match[0]['times_by_v'][v] / unit_div:.{RAW_DECIMALS}f}"
                            if match else ""
                        )
                    if any(values):
                        raw_rows.append([f"{op}--{fl}", v,
                                         SHORT_NAMES.get(lib, lib.lower())] + values)

    with open(RAW_OUTPUT_CSV, "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["benchmark", "V", "library"]
                   + [f"{m}_{RAW_UNIT}" for m in machines])
        w.writerows(raw_rows)

    print(f"Wrote {len(raw_rows)} rows to {RAW_OUTPUT_CSV}")


if __name__ == "__main__":
    main()