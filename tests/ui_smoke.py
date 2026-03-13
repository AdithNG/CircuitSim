#!/usr/bin/env python3

import sys
from pathlib import Path


def main() -> int:
    root = Path(__file__).resolve().parents[1]
    sys.path.insert(0, str(root / "python"))
    sys.path.insert(0, str(root / "build" / "python"))

    import app

    netlists = app.available_netlists()
    templates = app.available_templates()
    discovered = app.discover_module_dir()
    guidance = app.guidance_for_messages([{"code": "floating-nodes", "severity": "error", "message": "x"}])
    summary = {
        "analysis_type": "ac",
        "node_count": 3,
        "component_count": 3,
        "resistor_count": 1,
        "capacitor_count": 1,
        "inductor_count": 0,
        "voltage_source_count": 1,
        "current_source_count": 0,
        "sample_count": 5,
        "time_step": 0.0,
        "stop_time": 0.0,
    }

    if "AC low-pass" not in netlists:
        raise SystemExit("expected AC low-pass example in app")
    if "On-chip interconnect" not in templates:
        raise SystemExit("expected showcase template in app")
    if not (root / "python" / "app.py").exists():
        raise SystemExit("expected Streamlit app file to exist")
    if discovered.name not in {"python", "Debug"}:
        raise SystemExit(f"unexpected module discovery path: {discovered}")
    if not guidance or "Connect isolated nodes" not in guidance[0]:
        raise SystemExit(f"unexpected guidance output: {guidance}")
    if app.plot_ac_phase([1.0, 10.0], {"out": [{"phase_rad": 0.0}, {"phase_rad": -1.0}]}, "test") is None:
        raise SystemExit("expected AC phase plot helper to return a figure")
    if app.guidance_for_messages([]) != []:
        raise SystemExit("expected no guidance for an empty message list")
    if summary["analysis_type"] != "ac":
        raise SystemExit("summary smoke setup failed")

    print("UI smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
