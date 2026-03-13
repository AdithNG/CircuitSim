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

    if "AC low-pass" not in netlists:
        raise SystemExit("expected AC low-pass example in app")
    if "On-chip interconnect" not in templates:
        raise SystemExit("expected showcase template in app")
    if not (root / "python" / "app.py").exists():
        raise SystemExit("expected Streamlit app file to exist")

    print("UI smoke test passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
