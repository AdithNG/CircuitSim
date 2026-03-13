# CircuitSim

CircuitSim is a simulation-first circuit analysis and design exploration tool built for EDA, semiconductor, and systems software workflows. It combines a C++ solver core with Python bindings, scripting utilities, and a Streamlit workbench for interactive use.

## Features

- SPICE-like netlist parsing and validation
- DC operating point analysis
- transient RC and RL simulation
- AC small-signal analysis for linear RLC circuits
- Python bindings for direct scripting
- parameter sweeps with JSON export and plotting
- topology diagnostics for common netlist issues
- per-analysis summaries for solver runs
- a chip-focused interconnect showcase
- automated tests and GitHub Actions CI

## Quick Start

Install dependencies and launch the app:

```bash
python -m pip install -r requirements.txt
make ui
```

Then open:

```text
http://localhost:8501
```

`make ui` handles CMake configure/build automatically through the `Makefile`, so the CMake commands do not need to be run separately for normal app usage.

## How It Works

The user-facing app is written in Python, but it depends on a compiled C++ backend.

- The C++ layer implements the parser and solver core.
- The Python extension module `circuitsim_py` exposes that core to scripts and the UI.
- The Streamlit app provides the main interactive entry point.

## Streamlit Workbench

The app lives in [python/app.py](python/app.py).

Available views:

- DC analysis
- transient analysis
- AC analysis
- parameter sweeps
- diagnostics
- chip showcase

The app auto-detects the built Python extension by default.

## Current Coverage

CircuitSim currently includes:

- netlist parsing and validation
- DC analysis with steady-state capacitor and inductor handling
- transient analysis for RC and RL circuits
- AC small-signal analysis for linear RLC circuits
- Python bindings, plotting utilities, parameter sweeps, and diagnostics
- solver metadata summaries in the CLI and Python API
- a Streamlit workbench and a chip-focused interconnect showcase

Areas still worth extending:

- deeper convergence diagnostics
- richer simulation summaries and metadata
- performance profiling and optimization

## Common Commands

Run tests:

```bash
python -m pip install -r requirements.txt
make test
```

Run the CLI:

```bash
make run-dc
make run-tran
make run-ac
```

Each CLI analysis prints a short summary before the numeric results, including the
analysis type, node/component counts, component mix, and sample count.

Run a sweep:

```bash
make sweep-dc
```

Run the chip showcase:

```bash
make showcase
```

## Manual Build

If you want the lower-level CMake flow directly:

```bash
python -m pip install -r requirements.txt
cmake -S . -B build
cmake --build build --config Debug
```

## Python API

After building, the extension module is available in `build/python`.

```python
import sys
sys.path.insert(0, "build/python")

import circuitsim_py

dc = circuitsim_py.run_dc("V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n")
print(dc["summary"])
print(dc["node_voltages"]["out"])

ac = circuitsim_py.run_ac("V1 in 0 1\nR1 in out 1k\nC1 out 0 1n\n", [159154.94309189535])
print(ac["node_voltages"]["out"][0]["magnitude"])
```

## Chip Showcase

CircuitSim includes a simplified on-chip interconnect study that sweeps wire resistance in an RC network and measures 50% delay at the receiving node.

Outputs:

- `build/chip_showcase.json`
- `plots/chip_showcase_waveforms.png`
- `plots/chip_showcase_delay.png`

Additional notes live in [docs/chip_showcase.md](docs/chip_showcase.md).

## Repository Layout

```text
CircuitSim/
  README.md
  TASKS.md
  Makefile
  CMakeLists.txt
  include/
  src/
  python/
  tests/
  examples/
  docs/
```

## Roadmap

The detailed task plan lives in [TASKS.md](TASKS.md).
