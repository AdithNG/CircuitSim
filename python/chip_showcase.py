#!/usr/bin/env python3

import argparse
import json
import sys
from pathlib import Path

from sweep import load_module, render_template

ROOT = Path(__file__).resolve().parents[1]


def parse_values(text: str) -> list[float]:
    values = []
    for chunk in text.split(","):
        chunk = chunk.strip()
        if chunk:
            values.append(float(chunk))
    if not values:
        raise ValueError("at least one resistance value is required")
    return values


def crossing_time(time_points: list[float], waveform: list[float], threshold: float) -> float | None:
    for index, value in enumerate(waveform):
        if value >= threshold:
            if index == 0:
                return time_points[0]
            prev_t = time_points[index - 1]
            prev_v = waveform[index - 1]
            next_t = time_points[index]
            next_v = value
            if next_v == prev_v:
                return next_t
            fraction = (threshold - prev_v) / (next_v - prev_v)
            return prev_t + fraction * (next_t - prev_t)
    return None


def plot_waveforms(results: list[dict], output_path: Path) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure, axis = plt.subplots(figsize=(8, 4.5))
    for item in results:
        axis.plot(
            item["time_points"],
            item["waveform"],
            label=f"RWIRE={item['wire_resistance']:.0f} ohm",
        )
    axis.set_title("On-Chip Interconnect Step Response")
    axis.set_xlabel("Time (s)")
    axis.set_ylabel("Receiver voltage (V)")
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    axis.legend()
    figure.tight_layout()
    figure.savefig(output_path, dpi=160)
    plt.close(figure)


def plot_delay_curve(results: list[dict], output_path: Path) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure, axis = plt.subplots(figsize=(7, 4.2))
    axis.plot(
        [item["wire_resistance"] for item in results],
        [item["delay_50pct"] * 1e12 for item in results],
        marker="o",
    )
    axis.set_title("Interconnect Delay vs Wire Resistance")
    axis.set_xlabel("Wire resistance (ohm)")
    axis.set_ylabel("50% delay (ps)")
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    figure.tight_layout()
    figure.savefig(output_path, dpi=160)
    plt.close(figure)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run the chip-focused CircuitSim showcase.")
    parser.add_argument(
        "--template",
        default=str(ROOT / "examples" / "on_chip_interconnect.cir.in"),
    )
    parser.add_argument(
        "--module-dir",
        default=str(ROOT / "build" / "python"),
    )
    parser.add_argument(
        "--wire-values",
        default="50,100,200,400",
    )
    parser.add_argument(
        "--time-step",
        type=float,
        default=1e-11,
    )
    parser.add_argument(
        "--stop-time",
        type=float,
        default=6e-9,
    )
    parser.add_argument(
        "--threshold",
        type=float,
        default=0.5,
    )
    parser.add_argument(
        "--output-json",
        default=str(ROOT / "build" / "chip_showcase.json"),
    )
    parser.add_argument(
        "--waveform-plot",
        default=str(ROOT / "plots" / "chip_showcase_waveforms.png"),
    )
    parser.add_argument(
        "--delay-plot",
        default=str(ROOT / "plots" / "chip_showcase_delay.png"),
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    circuitsim_py = load_module(args.module_dir)
    template_text = Path(args.template).read_text()
    wire_values = parse_values(args.wire_values)

    results = []
    for wire_resistance in wire_values:
        rendered = render_template(template_text, {"RWIRE": wire_resistance})
        transient = circuitsim_py.run_transient(rendered, args.time_step, args.stop_time)
        waveform = transient["node_voltages"]["out"]
        delay = crossing_time(transient["time_points"], waveform, args.threshold)
        results.append(
            {
                "wire_resistance": wire_resistance,
                "threshold": args.threshold,
                "delay_50pct": delay,
                "final_voltage": waveform[-1],
                "time_points": transient["time_points"],
                "waveform": waveform,
            }
        )

    payload = {
        "scenario": "on_chip_interconnect",
        "threshold": args.threshold,
        "time_step": args.time_step,
        "stop_time": args.stop_time,
        "results": results,
    }

    output_json = Path(args.output_json)
    output_json.parent.mkdir(parents=True, exist_ok=True)
    output_json.write_text(json.dumps(payload, indent=2))

    plot_waveforms(results, Path(args.waveform_plot))
    plot_delay_curve(results, Path(args.delay_plot))

    print(f"Wrote showcase results to {args.output_json}")
    print(f"Wrote waveform plot to {args.waveform_plot}")
    print(f"Wrote delay plot to {args.delay_plot}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
