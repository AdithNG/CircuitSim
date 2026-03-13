#!/usr/bin/env python3

import argparse
import sys
from pathlib import Path


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser()
    parser.add_argument("--module-dir", required=True)
    return parser.parse_args()


def main() -> int:
    args = parse_args()
    sys.path.insert(0, args.module_dir)

    import circuitsim_py

    dc_result = circuitsim_py.run_dc("V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n")
    out_voltage = dc_result["node_voltages"]["out"]
    if abs(out_voltage - (10.0 / 3.0)) > 1e-9:
        raise SystemExit(f"unexpected DC out voltage: {out_voltage}")

    tran_result = circuitsim_py.run_transient(
        "V1 in 0 5\nR1 in out 1k\nC1 out 0 1u\n",
        1e-4,
        5e-3,
    )
    final_out = tran_result["node_voltages"]["out"][-1]
    if final_out < 4.9 or final_out > 5.01:
        raise SystemExit(f"unexpected transient final out voltage: {final_out}")

    parsed = circuitsim_py.parse_netlist("V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n")
    if parsed["nodes"] != ["in", "0", "out"]:
        raise SystemExit(f"unexpected node list: {parsed['nodes']}")

    print("Python bindings smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
