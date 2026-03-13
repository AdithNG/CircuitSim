# CircuitSim

CircuitSim is a simulation-first circuit analysis and design exploration tool aimed at EDA, semiconductor, and systems software roles. It combines a C++ solver core with a Python automation layer and a Streamlit UI for interactive demos.

## Quick Start

The main way to use this project is through the Streamlit app.

Install Python dependencies, build the C++ extension once, then launch the app:

```bash
python -m pip install -r requirements.txt
cmake -S . -B build
cmake --build build --config Debug
make ui
```

Then open:

```text
http://localhost:8501
```

## Important Note

The app is written in Python, but it still depends on the compiled C++ backend.

That means:

- you do run a Python app
- but you still need to build the C++ project first
- because the UI calls into the compiled `circuitsim_py` extension in `build/python`

So the Python app is the frontend/workflow layer, and the C++ code is the simulation engine underneath it.

## What You Built

CircuitSim currently includes:

- SPICE-like netlist parsing and validation
- DC operating point solving
- transient simulation for RC circuits
- AC small-signal analysis for linear RC circuits
- Python bindings for the simulation APIs
- parameterized sweeps with JSON export and plotting
- diagnostics for missing ground, floating nodes, and reactive-only networks
- a chip-focused interconnect showcase
- a Streamlit workbench for interactive use
- automated tests and GitHub Actions CI

## Streamlit Workbench

The app lives in [python/app.py](python/app.py).

It currently provides:

- DC analysis
- transient analysis
- AC analysis
- parameter sweeps
- diagnostics
- the chip-focused showcase

The app auto-detects the built Python extension by default, so you normally do not need to configure any module path manually.

## What This Project Demonstrates

CircuitSim is designed to show strength in:

- numerical methods and matrix-based simulation
- parser and intermediate-representation design
- performance-conscious C++ engineering
- Python and C++ interoperability
- tooling and workflow design for technical users

## Project Status

Most of the original planned milestones are complete in a solid first version.

Completed or mostly completed:

- repository setup and test scaffolding
- netlist parser and circuit graph
- DC solver
- transient RC solver
- Python automation layer
- parameter sweeps and experiment runner
- diagnostics and workflow reporting
- chip-focused showcase example
- CI and interactive UI

Still partial or not done yet:

- RL transient support
- inductor support in DC, transient, and AC analysis
- deeper convergence diagnostics
- richer simulation summaries and metadata
- performance profiling and optimization pass

## Build And Test

```bash
python -m pip install -r requirements.txt
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Or with the `Makefile`:

```bash
python -m pip install -r requirements.txt
make test
```

## Other Ways To Run It

Run the CLI directly:

```bash
./build/Debug/circuitsim_cli examples/resistor_divider.cir
./build/Debug/circuitsim_cli tran examples/rc_charge.cir 1e-4 5e-3
./build/Debug/circuitsim_cli ac examples/ac_lowpass.cir 159154.94309189535
```

Run a sweep:

```bash
make sweep-dc
```

Run the chip showcase:

```bash
make showcase
```

## Python API

After building, the Python extension module is available in `build/python`.

Example:

```python
import sys
sys.path.insert(0, "build/python")

import circuitsim_py

dc = circuitsim_py.run_dc("V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n")
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

The detailed write-up lives in [docs/chip_showcase.md](docs/chip_showcase.md).

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
