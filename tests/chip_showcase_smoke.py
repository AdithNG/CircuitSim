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
    script = root / "python" / "chip_showcase.py"
    output_json = root / "build" / "chip_showcase_smoke.json"
    waveform_plot = root / "build" / "chip_showcase_waveforms.png"
    delay_plot = root / "build" / "chip_showcase_delay.png"

    result = subprocess.run(
        [
            sys.executable,
            str(script),
            "--module-dir",
            args.module_dir,
            "--output-json",
            str(output_json),
            "--waveform-plot",
            str(waveform_plot),
            "--delay-plot",
            str(delay_plot),
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)

    payload = json.loads(output_json.read_text())
    delays = [item["delay_50pct"] for item in payload["results"]]
    finals = [item["final_voltage"] for item in payload["results"]]

    if any(delay is None for delay in delays):
        raise SystemExit("expected all delays to be measurable")
    if not all(delays[index] < delays[index + 1] for index in range(len(delays) - 1)):
        raise SystemExit(f"expected delays to increase with wire resistance, got {delays}")
    if not all(final > 0.99 for final in finals):
        raise SystemExit(f"expected final voltages near 1V, got {finals}")
    if not waveform_plot.exists() or not delay_plot.exists():
        raise SystemExit("expected showcase plot files to be created")

    print("Chip showcase smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
