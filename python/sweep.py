#!/usr/bin/env python3

import argparse
import json
import re
import sys
from pathlib import Path


PLACEHOLDER_PATTERN = re.compile(r"\{\{([A-Za-z_][A-Za-z0-9_]*)\}\}")


def render_template(template: str, parameters: dict[str, float]) -> str:
    def replace(match: re.Match[str]) -> str:
        name = match.group(1)
        if name not in parameters:
            raise KeyError(f"missing parameter '{name}'")
        return format(parameters[name], ".12g")

    return PLACEHOLDER_PATTERN.sub(replace, template)


def parse_values(text: str) -> list[float]:
    values = []
    for chunk in text.split(","):
        chunk = chunk.strip()
        if not chunk:
            continue
        values.append(float(chunk))
    if not values:
        raise ValueError("at least one sweep value is required")
    return values


def load_module(module_dir: str):
    sys.path.insert(0, module_dir)
    import circuitsim_py

    return circuitsim_py


def run_dc_sweep(
    circuitsim_py,
    template: str,
    parameter_name: str,
    values: list[float],
    observed_node: str,
) -> list[dict]:
    results = []
    for value in values:
        rendered = render_template(template, {parameter_name: value})
        dc_result = circuitsim_py.run_dc(rendered)
        results.append(
            {
                "parameter": value,
                "observed_node": observed_node,
                "node_voltage": dc_result["node_voltages"][observed_node],
                "node_voltages": dc_result["node_voltages"],
                "source_currents": dc_result["source_currents"],
            }
        )
    return results


def run_transient_sweep(
    circuitsim_py,
    template: str,
    parameter_name: str,
    values: list[float],
    observed_node: str,
    time_step: float,
    stop_time: float,
) -> list[dict]:
    results = []
    for value in values:
        rendered = render_template(template, {parameter_name: value})
        transient_result = circuitsim_py.run_transient(rendered, time_step, stop_time)
        series = transient_result["node_voltages"][observed_node]
        results.append(
            {
                "parameter": value,
                "observed_node": observed_node,
                "final_voltage": series[-1],
                "time_points": transient_result["time_points"],
                "waveform": series,
            }
        )
    return results


def write_json(output_path: Path, payload: dict) -> None:
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(json.dumps(payload, indent=2))


def plot_dc_sweep(results: list[dict], parameter_name: str, output_path: Path) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure, axis = plt.subplots(figsize=(7, 4.2))
    axis.plot(
        [item["parameter"] for item in results],
        [item["node_voltage"] for item in results],
        marker="o",
    )
    axis.set_xlabel(parameter_name)
    axis.set_ylabel("Observed node voltage (V)")
    axis.set_title("DC Parameter Sweep")
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    figure.tight_layout()
    figure.savefig(output_path, dpi=160)
    plt.close(figure)


def plot_transient_sweep(results: list[dict], parameter_name: str, output_path: Path) -> None:
    import matplotlib

    matplotlib.use("Agg")
    import matplotlib.pyplot as plt

    output_path.parent.mkdir(parents=True, exist_ok=True)
    figure, axis = plt.subplots(figsize=(8, 4.5))
    for item in results:
        axis.plot(
            item["time_points"],
            item["waveform"],
            label=f"{parameter_name}={item['parameter']:.6g}",
        )
    axis.set_xlabel("Time (s)")
    axis.set_ylabel("Observed node voltage (V)")
    axis.set_title("Transient Parameter Sweep")
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    axis.legend()
    figure.tight_layout()
    figure.savefig(output_path, dpi=160)
    plt.close(figure)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run a parameter sweep with CircuitSim.")
    parser.add_argument("mode", choices=["dc", "tran"])
    parser.add_argument("template", help="Path to the parameterized netlist template")
    parser.add_argument("parameter", help="Parameter placeholder name, e.g. RLOAD")
    parser.add_argument("values", help="Comma-separated parameter values")
    parser.add_argument("--module-dir", default=str(Path("build") / "python"))
    parser.add_argument("--observe", required=True, help="Node name to observe")
    parser.add_argument("--output-json", default=str(Path("build") / "sweep_results.json"))
    parser.add_argument("--plot", default=str(Path("plots") / "sweep.png"))
    parser.add_argument("--time-step", type=float)
    parser.add_argument("--stop-time", type=float)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    circuitsim_py = load_module(args.module_dir)
    template_text = Path(args.template).read_text()
    values = parse_values(args.values)

    payload = {
        "mode": args.mode,
        "parameter": args.parameter,
        "values": values,
        "observed_node": args.observe,
    }

    if args.mode == "dc":
        results = run_dc_sweep(circuitsim_py, template_text, args.parameter, values, args.observe)
        payload["results"] = results
        write_json(Path(args.output_json), payload)
        plot_dc_sweep(results, args.parameter, Path(args.plot))
    else:
        if args.time_step is None or args.stop_time is None:
            raise SystemExit("--time-step and --stop-time are required for transient sweeps")
        results = run_transient_sweep(
            circuitsim_py,
            template_text,
            args.parameter,
            values,
            args.observe,
            args.time_step,
            args.stop_time,
        )
        payload["time_step"] = args.time_step
        payload["stop_time"] = args.stop_time
        payload["results"] = results
        write_json(Path(args.output_json), payload)
        plot_transient_sweep(results, args.parameter, Path(args.plot))

    print(f"Wrote sweep results to {args.output_json}")
    print(f"Wrote sweep plot to {args.plot}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
