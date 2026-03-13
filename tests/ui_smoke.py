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

    print("UI smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
