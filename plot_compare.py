#!/usr/bin/env python3
import argparse
import os
import re
from pathlib import Path

os.environ.setdefault("MPLCONFIGDIR", "/tmp/matplotlib")
import matplotlib.pyplot as plt
import matplotlib.patheffects as pe


EXPECTED_COUNTS = [10 ** i for i in range(2, 10)]
XTICK_LABELS = [f"10^{i}" for i in range(2, 10)]

SERIES_CONFIG = {
    "random_fastslow": {
        "color": "#E69F00",
        "marker": "o",
        "linestyle": "-",
        "default_path": Path("logs/random_fastslow.txt"),
    },
    "random_single": {
        "color": "#56B4E9",
        "marker": "s",
        "linestyle": "--",
        "default_path": Path("logs/random_single.txt"),
    },
    "sequential_fastslow": {
        "color": "#009E73",
        "marker": "^",
        "linestyle": "-.",
        "default_path": Path("logs/sequential_fastslow.txt"),
    },
    "sequential_single": {
        "color": "#D55E00",
        "marker": "D",
        "linestyle": ":",
        "default_path": Path("logs/sequential_single.txt"),
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

METRIC_PATTERNS = {
    "task_clock_ns": re.compile(r"task-clock\(ns\):\s*(\d+)$"),
    "cpu_cycles": re.compile(r"cpu-cycles:\s*(\d+)$"),
    "cache_miss_rate": re.compile(r"cache-miss-rate:\s*([\d.]+)%$"),
    "l1_miss_rate": re.compile(r"L1-dcache-load-miss-rate:\s*([\d.]+)%$"),
    "l1_prefetches": re.compile(r"l1-dcache-prefetches:\s*(\d+)$"),
}

PLOT_TARGETS = [
    ("task_clock_ns", "task-clock(ns)"),
    ("cpu_cycles", "cpu-cycles"),
    ("cache_miss_rate", "cache-miss-rate (%)"),
    ("l1_miss_rate", "L1-dcache-load-miss-rate (%)"),
    ("l1_prefetches", "l1-dcache-prefetches"),
]

METRIC_AXIS_STYLE = {
    "task_clock_ns": {"yscale": "log"},
    "cpu_cycles": {"yscale": "log"},
    "cache_miss_rate": {"yscale": "linear"},
    "l1_miss_rate": {"yscale": "linear"},
    "l1_prefetches": {"yscale": "linear", "negative_pad_ratio": 0.08},
}


def parse_metrics_file(path: Path):
    text = path.read_text(encoding="utf-8")
    data_by_count = {}
    current_count = None

    for raw_line in text.splitlines():
        line = raw_line.strip()
        if not line:
            continue

        m_count = re.search(r"count=(\d+)", line)
        if m_count:
            current_count = int(m_count.group(1))
            data_by_count.setdefault(current_count, {})
            continue

        if current_count is None:
            continue

        for metric_name, pattern in METRIC_PATTERNS.items():
            m = pattern.match(line)
            if not m:
                continue
            if metric_name in ("cache_miss_rate", "l1_miss_rate"):
                value = float(m.group(1))
            else:
                value = int(m.group(1))
            data_by_count[current_count][metric_name] = value
            break

    missing_counts = [c for c in EXPECTED_COUNTS if c not in data_by_count]
    if missing_counts:
        raise ValueError(f"{path}: missing count nodes {missing_counts}")

    for count in EXPECTED_COUNTS:
        missing_metrics = [
            metric for metric in METRIC_PATTERNS if metric not in data_by_count[count]
        ]
        if missing_metrics:
            raise ValueError(
                f"{path}: missing metrics for count={count}: {missing_metrics}"
            )

    series = {}
    for metric_name in METRIC_PATTERNS:
        series[metric_name] = [data_by_count[c][metric_name] for c in EXPECTED_COUNTS]
    return series


def plot_one_metric(all_series_data, metric_name, ylabel, out_path: Path):
    plt.figure(figsize=(12.5, 7.2))

    for z_index, series_name in enumerate(SERIES_ORDER, start=1):
        conf = SERIES_CONFIG[series_name]
        y = all_series_data[series_name][metric_name]
        x = [count * X_JITTER_FACTOR[series_name] for count in EXPECTED_COUNTS]
        (line,) = plt.plot(
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

    plt.xscale("log", base=10)
    plt.xticks(EXPECTED_COUNTS, XTICK_LABELS)

    axis_style = METRIC_AXIS_STYLE[metric_name]
    if axis_style["yscale"] == "log":
        plt.yscale("log")
    elif axis_style["yscale"] == "symlog":
        plt.yscale("symlog", linthresh=axis_style["linthresh"])
    if axis_style.get("negative_pad_ratio") is not None:
        y_max = max(max(all_series_data[name][metric_name]) for name in SERIES_ORDER)
        if y_max <= 0:
            plt.ylim(bottom=-1.0, top=1.0)
        else:
            pad = max(1.0, y_max * axis_style["negative_pad_ratio"])
            plt.ylim(bottom=-pad)

    plt.xlabel("number of nodes")
    plt.ylabel(ylabel)
    plt.minorticks_on()
    plt.grid(True, which="major", linestyle="--", linewidth=0.7, alpha=0.45)
    plt.grid(True, which="minor", linestyle=":", linewidth=0.5, alpha=0.28)
    plt.legend(ncol=2, frameon=True, framealpha=0.9, fontsize=10)
    plt.tight_layout()
    out_path.parent.mkdir(parents=True, exist_ok=True)
    plt.savefig(out_path, dpi=360)
    plt.close()


def main():
    parser = argparse.ArgumentParser(
        description="Plot 5 performance metric line charts from 4 benchmark logs."
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
