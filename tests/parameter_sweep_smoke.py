#!/usr/bin/env python3

import argparse
import json
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--module-dir", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    script = root / "python" / "sweep.py"
    output_json = root / "build" / "dc_sweep_smoke.json"
    output_plot = root / "build" / "dc_sweep_smoke.png"

    result = subprocess.run(
        [
            sys.executable,
            str(script),
            "dc",
            str(root / "examples" / "resistor_divider_param.cir.in"),
            "RLOAD",
            "1000,2000,4000",
            "--module-dir",
            args.module_dir,
            "--observe",
            "out",
            "--output-json",
            str(output_json),
            "--plot",
            str(output_plot),
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    if not output_json.exists():
        raise SystemExit("sweep json file was not created")
    if not output_plot.exists():
        raise SystemExit("sweep plot file was not created")

    payload = json.loads(output_json.read_text())
    voltages = [item["node_voltage"] for item in payload["results"]]
    if len(voltages) != 3:
        raise SystemExit("unexpected number of sweep results")
    if not (voltages[0] < voltages[1] < voltages[2]):
        raise SystemExit(f"expected monotonic increase in out voltage, got {voltages}")

    print("Parameter sweep smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
