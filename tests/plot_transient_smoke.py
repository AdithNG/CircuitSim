#!/usr/bin/env python3

import argparse
import subprocess
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--cli", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    root = Path(__file__).resolve().parents[1]
    script = root / "python" / "plot_transient.py"
    netlist = root / "examples" / "rc_charge.cir"
    output = root / "build" / "transient_smoke.png"

    result = subprocess.run(
        [
            sys.executable,
            str(script),
            str(netlist),
            "1e-4",
            "5e-3",
            "--cli",
            args.cli,
            "--output",
            str(output),
            "--no-show",
        ],
        capture_output=True,
        text=True,
        check=False,
    )

    if result.returncode != 0:
        raise SystemExit(result.stderr or result.stdout)
    if not output.exists():
        raise SystemExit("plot file was not created")
    print(result.stdout.strip())
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
