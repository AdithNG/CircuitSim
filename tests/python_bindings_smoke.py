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
    if dc_result["summary"]["analysis_type"] != "dc":
        raise SystemExit(f"unexpected DC analysis type: {dc_result['summary']['analysis_type']}")
    if dc_result["summary"]["component_count"] != 3:
        raise SystemExit(f"unexpected DC component count: {dc_result['summary']['component_count']}")
    out_voltage = dc_result["node_voltages"]["out"]
    if abs(out_voltage - (10.0 / 3.0)) > 1e-9:
        raise SystemExit(f"unexpected DC out voltage: {out_voltage}")

    tran_result = circuitsim_py.run_transient(
        "V1 in 0 5\nR1 in out 1k\nC1 out 0 1u\n",
        1e-4,
        5e-3,
    )
    if tran_result["summary"]["analysis_type"] != "transient":
        raise SystemExit(
            f"unexpected transient analysis type: {tran_result['summary']['analysis_type']}"
        )
    if tran_result["summary"]["sample_count"] != len(tran_result["time_points"]):
        raise SystemExit("transient summary sample count does not match waveform length")
    final_out = tran_result["node_voltages"]["out"][-1]
    if final_out < 4.9 or final_out > 5.01:
        raise SystemExit(f"unexpected transient final out voltage: {final_out}")

    ac_result = circuitsim_py.run_ac(
        "V1 in 0 1\nR1 in out 1k\nC1 out 0 1n\n",
        [159154.94309189535],
    )
    if ac_result["summary"]["analysis_type"] != "ac":
        raise SystemExit(f"unexpected AC analysis type: {ac_result['summary']['analysis_type']}")
    if ac_result["summary"]["sample_count"] != 1:
        raise SystemExit(f"unexpected AC sample count: {ac_result['summary']['sample_count']}")
    ac_out = ac_result["node_voltages"]["out"][0]["magnitude"]
    if abs(ac_out - (2 ** -0.5)) > 1e-3:
        raise SystemExit(f"unexpected AC out magnitude: {ac_out}")

    parsed = circuitsim_py.parse_netlist("V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n")
    if parsed["nodes"] != ["in", "0", "out"]:
        raise SystemExit(f"unexpected node list: {parsed['nodes']}")

    diagnostics = circuitsim_py.diagnose_netlist("R1 a b 1k\n")
    if not diagnostics["has_errors"]:
        raise SystemExit("expected diagnostics to report an error for missing ground")
    if diagnostics["messages"][0]["code"] != "missing-ground":
        raise SystemExit(f"unexpected diagnostic code: {diagnostics['messages'][0]['code']}")

    print("Python bindings smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
