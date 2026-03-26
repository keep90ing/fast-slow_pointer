#!/usr/bin/env python3
import argparse
import csv
import os
from statistics import median
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/matplotlib")
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe


EXPECTED_COUNTS = [10 ** i for i in range(3, 10)]
XTICK_LABELS = [f"10^{i}" for i in range(3, 10)]

SERIES_CONFIG = {
    "random_fastslow": {
        "color": "#E69F00",
        "marker": "o",
        "linestyle": "-",
        "default_path": Path("logs/random_fastslow.csv"),
    },
    "random_single": {
        "color": "#56B4E9",
        "marker": "s",
        "linestyle": "--",
        "default_path": Path("logs/random_single.csv"),
    },
    "sequential_fastslow": {
        "color": "#009E73",
        "marker": "^",
        "linestyle": "-.",
        "default_path": Path("logs/sequential_fastslow.csv"),
    },
    "sequential_single": {
        "color": "#D55E00",
        "marker": "D",
        "linestyle": ":",
        "default_path": Path("logs/sequential_single.csv"),
    },
}

SERIES_ORDER = [
    "random_fastslow",
    "random_single",
    "sequential_fastslow",
    "sequential_single",
]

# Slight horizontal offsets keep near-overlapping series distinguishable.
X_JITTER_FACTOR = {
    "random_fastslow": 0.97,
    "random_single": 0.99,
    "sequential_fastslow": 1.01,
    "sequential_single": 1.03,
}

CSV_COLUMNS = {
    "cpu_cycles": ("cpu_cycles",),
    "cache_miss_rate": ("cache_miss_rate",),
    "l1_miss_rate": ("l1_dcache_load_miss_rate",),
    "l1_prefetches": ("l1_dcache_prefetches",),
    "dtlb_miss_rate": ("dtlb_load_miss_rate",),
}

OPTIONAL_METRICS = {"l1_prefetches", "dtlb_miss_rate"}

PLOT_TARGETS = [
    ("cpu_cycles", "cpu-cycles"),
    ("cache_miss_rate", "cache-miss-rate (%)"),
    ("l1_miss_rate", "L1-dcache-load-miss-rate (%)"),
    ("l1_prefetches", "l1-dcache-prefetches"),
    ("dtlb_miss_rate", "DTLB-load-miss-rate (%)"),
]

METRIC_AXIS_STYLE = {
    "cpu_cycles": {"yscale": "log"},
    "cache_miss_rate": {"yscale": "linear"},
    "l1_miss_rate": {"yscale": "linear"},
    "l1_prefetches": {"yscale": "linear", "negative_pad_ratio": 0.08},
    "dtlb_miss_rate": {"yscale": "linear"},
}


def parse_metrics_file(path: Path):
    data_by_count = {
        count: {metric_name: [] for metric_name in CSV_COLUMNS}
        for count in EXPECTED_COUNTS
    }

    with path.open(newline="", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        if not reader.fieldnames:
            raise ValueError(f"{path}: empty CSV")
        if "count" not in reader.fieldnames:
            raise ValueError(f"{path}: missing CSV column 'count'")

        metric_column_map = {}
        for metric_name, candidates in CSV_COLUMNS.items():
            found = None
            for col in candidates:
                if col in reader.fieldnames:
                    found = col
                    break
            if not found:
                if metric_name in OPTIONAL_METRICS:
                    metric_column_map[metric_name] = None
                    continue
                raise ValueError(
                    f"{path}: missing CSV columns for '{metric_name}', "
                    f"expected one of {candidates}"
                )
            metric_column_map[metric_name] = found

        for row in reader:
            if not row:
                continue
            try:
                count = int(row["count"])
            except (TypeError, ValueError):
                continue
            if count not in data_by_count:
                continue

            for metric_name, csv_col in metric_column_map.items():
                if csv_col is None:
                    continue
                raw = (row.get(csv_col) or "").strip()
                if not raw or raw == "N/A":
                    continue
                try:
                    value = float(raw)
                except ValueError:
                    continue
                data_by_count[count][metric_name].append(value)

    missing_counts = [
        c
        for c in EXPECTED_COUNTS
        if not any(data_by_count[c][metric] for metric in CSV_COLUMNS)
    ]
    if missing_counts:
        raise ValueError(f"{path}: missing count nodes {missing_counts}")

    for count in EXPECTED_COUNTS:
        missing_metrics = []
        for metric_name in CSV_COLUMNS:
            if data_by_count[count][metric_name]:
                continue
            if metric_name in OPTIONAL_METRICS:
                data_by_count[count][metric_name] = [0.0]
                continue
            missing_metrics.append(metric_name)
        if missing_metrics:
            raise ValueError(
                f"{path}: missing metrics for count={count}: {missing_metrics}"
            )

    series = {}
    for metric_name in CSV_COLUMNS:
        series[metric_name] = [median(data_by_count[c][metric_name]) for c in EXPECTED_COUNTS]
    return series


def draw_metric_on_axis(ax, all_series_data, metric_name, ylabel, show_legend=True):
    for z_index, series_name in enumerate(SERIES_ORDER, start=1):
        conf = SERIES_CONFIG[series_name]
        y = all_series_data[series_name][metric_name]
        x = [count * X_JITTER_FACTOR[series_name] for count in EXPECTED_COUNTS]
        (line,) = ax.plot(
            x,
            y,
            marker=conf["marker"],
            linestyle=conf["linestyle"],
            linewidth=1.15,
            markersize=5.8,
            markerfacecolor="white",
            markeredgewidth=1.2,
            alpha=0.98,
            color=conf["color"],
            label=series_name,
            zorder=10 + z_index,
            antialiased=True,
            solid_capstyle="round",
            solid_joinstyle="round",
        )
        line.set_path_effects(
            [pe.Stroke(linewidth=2.8, foreground="white", alpha=0.9), pe.Normal()]
        )

    ax.set_xscale("log", base=10)
    ax.set_xticks(EXPECTED_COUNTS)
    ax.set_xticklabels(XTICK_LABELS)

    axis_style = METRIC_AXIS_STYLE[metric_name]
    if axis_style["yscale"] == "log":
        ax.set_yscale("log")
    elif axis_style["yscale"] == "symlog":
        ax.set_yscale("symlog", linthresh=axis_style["linthresh"])
    if axis_style.get("negative_pad_ratio") is not None:
        y_max = max(max(all_series_data[name][metric_name]) for name in SERIES_ORDER)
        if y_max <= 0:
            ax.set_ylim(bottom=-1.0, top=1.0)
        else:
            pad = max(1.0, y_max * axis_style["negative_pad_ratio"])
            ax.set_ylim(bottom=-pad)

    ax.set_xlabel("number of nodes")
    ax.set_ylabel(ylabel)
    ax.minorticks_on()
    ax.grid(True, which="major", linestyle="--", linewidth=0.7, alpha=0.45)
    ax.grid(True, which="minor", linestyle=":", linewidth=0.5, alpha=0.28)
    if show_legend:
        ax.legend(ncol=2, frameon=True, framealpha=0.9, fontsize=10)


def plot_one_metric(all_series_data, metric_name, ylabel, out_path: Path):
    fig, ax = plt.subplots(figsize=(12.5, 7.2))
    draw_metric_on_axis(ax, all_series_data, metric_name, ylabel, show_legend=True)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    fig.tight_layout()
    fig.savefig(out_path, dpi=360)
    plt.close(fig)


def main():
    parser = argparse.ArgumentParser(
        description="Plot metric charts from 4 benchmark CSV files using per-count medians."
    )
    parser.add_argument(
        "--random-fastslow",
        type=Path,
        default=SERIES_CONFIG["random_fastslow"]["default_path"],
    )
    parser.add_argument(
        "--random-single",
        type=Path,
        default=SERIES_CONFIG["random_single"]["default_path"],
    )
    parser.add_argument(
        "--sequential-fastslow",
        type=Path,
        default=SERIES_CONFIG["sequential_fastslow"]["default_path"],
    )
    parser.add_argument(
        "--sequential-single",
        type=Path,
        default=SERIES_CONFIG["sequential_single"]["default_path"],
    )
    parser.add_argument(
        "-o",
        "--out-dir",
        type=Path,
        default=Path("logs"),
        help="Output directory for PNG files (default: logs)",
    )
    args = parser.parse_args()

    input_paths = {
        "random_fastslow": args.random_fastslow,
        "random_single": args.random_single,
        "sequential_fastslow": args.sequential_fastslow,
        "sequential_single": args.sequential_single,
    }

    all_series_data = {}
    for series_name, path in input_paths.items():
        all_series_data[series_name] = parse_metrics_file(path)

    generated = []
    for metric_name, ylabel in PLOT_TARGETS:
        out_path = args.out_dir / f"{metric_name}_compare_4way.png"
        plot_one_metric(all_series_data, metric_name, ylabel, out_path)
        generated.append(out_path)

    for series_name, path in input_paths.items():
        print(f"{series_name}={path}")
    for out_path in generated:
        print(f"generated={out_path}")


if __name__ == "__main__":
    main()
