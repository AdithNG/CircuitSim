#!/usr/bin/env python3

import argparse
import csv
import io
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run a transient simulation and plot node voltages."
    )
    parser.add_argument("netlist", help="Path to the input netlist file")
    parser.add_argument("time_step", help="Transient time step")
    parser.add_argument("stop_time", help="Transient stop time")
    parser.add_argument(
        "--cli",
        default=str(
            (Path("build") / "Debug" / "circuitsim_cli.exe")
            if sys.platform.startswith("win")
            else (Path("build") / "circuitsim_cli")
        ),
        help="Path to the CircuitSim CLI executable",
    )
    parser.add_argument(
        "--output",
        default=str(Path("plots") / "transient.png"),
        help="Path to write the output plot image",
    )
    parser.add_argument(
        "--title",
        default="CircuitSim Transient Response",
        help="Plot title",
    )
    parser.add_argument(
        "--no-show",
        action="store_true",
        help="Save the plot without opening a window",
    )
    return parser.parse_args()


def run_cli(cli_path: str, netlist: str, time_step: str, stop_time: str) -> str:
    result = subprocess.run(
        [cli_path, "tran", netlist, time_step, stop_time],
        capture_output=True,
        text=True,
        check=False,
    )
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or "transient CLI command failed")
    return result.stdout


def parse_csv(csv_text: str) -> tuple[list[float], dict[str, list[float]]]:
    reader = csv.DictReader(io.StringIO(csv_text))
    times: list[float] = []
    series: dict[str, list[float]] = {}

    for row in reader:
        times.append(float(row["time"]))
        for key, value in row.items():
            if key == "time":
                continue
            series.setdefault(key, []).append(float(value))

    if not times:
        raise RuntimeError("no transient samples were produced")

    return times, series


def plot_series(
    times: list[float],
    series: dict[str, list[float]],
    output_path: Path,
    title: str,
    no_show: bool,
) -> None:
    import matplotlib

    if no_show:
        matplotlib.use("Agg")

    import matplotlib.pyplot as plt

    output_path.parent.mkdir(parents=True, exist_ok=True)

    figure, axis = plt.subplots(figsize=(8, 4.5))
    for node_name, values in sorted(series.items()):
        axis.plot(times, values, label=node_name)

    axis.set_title(title)
    axis.set_xlabel("Time (s)")
    axis.set_ylabel("Voltage (V)")
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    axis.legend()
    figure.tight_layout()
    figure.savefig(output_path, dpi=160)

    if not no_show:
        plt.show()
    plt.close(figure)


def main() -> int:
    args = parse_args()
    csv_output = run_cli(args.cli, args.netlist, args.time_step, args.stop_time)
    times, series = parse_csv(csv_output)
    plot_series(times, series, Path(args.output), args.title, args.no_show)
    print(f"Wrote plot to {args.output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
