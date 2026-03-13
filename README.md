# CircuitSim

CircuitSim is a simulation-first circuit analysis project aimed at EDA, semiconductor, and systems software roles. It focuses on building a real circuit solver with a clean architecture, strong testing, and a Python-friendly workflow layer for design exploration.

## Goals

- Parse SPICE-like netlists into a validated circuit model
- Simulate circuits with DC, transient, and later AC analysis
- Expose the simulator through a scriptable automation layer
- Add workflow features such as parameter sweeps and diagnostics

## Why This Project

CircuitSim is meant to demonstrate:

- numerical methods and matrix-based simulation
- compiler-like parsing and validation
- performance-conscious C++ design
- Python interoperability for automation
- engineering tooling for technical users

## Planned Scope

Initial milestones:

1. Netlist parser and circuit graph
2. DC operating point solver
3. Transient simulation for RC and RL circuits
4. Python bindings for scripted experiments
5. Parameter sweeps and diagnostics

Stretch goals:

- AC small-signal analysis
- chip-focused RC or RLC interconnect studies
- optimization-driven design exploration
- waveform comparison and reporting tools

## Project Status

The repository is in active early development.

Implemented so far:

- SPICE-like netlist parsing and validation
- DC operating point solving for resistors, current sources, and voltage sources
- transient simulation for resistor-capacitor circuits using backward Euler
- AC small-signal analysis for linear RC circuits
- a small CLI for running DC or transient analysis on a netlist file
- Python bindings for parsing, DC analysis, and transient simulation
- parameterized netlist sweeps with JSON export and comparison plotting
- circuit diagnostics for missing ground, floating nodes, and reactive-only warnings
- a chip-focused interconnect showcase with waveform and delay analysis
- automated tests and CI

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

## Development Principles

- Build in small, testable slices
- Prefer correctness and diagnostics before optimization
- Keep the core architecture clean and extensible
- Add tests for both successful and failure cases

## Build And Test

```bash
python -m pip install -r requirements.txt
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

Or with the convenience `Makefile`:

```bash
python -m pip install -r requirements.txt
make test
```

## Run The CLI

```bash
./build/Debug/circuitsim_cli examples/resistor_divider.cir
```

Transient example:

```bash
./build/Debug/circuitsim_cli tran examples/rc_charge.cir 1e-4 5e-3
```

AC example:

```bash
./build/Debug/circuitsim_cli ac examples/ac_lowpass.cir 159154.94309189535
```

## Plot A Transient Response

```bash
python python/plot_transient.py examples/rc_charge.cir 1e-4 5e-3 --output plots/rc_charge.png --no-show
```

This uses `matplotlib` on top of the C++ CLI output, which gives us a fast path to visuals before we add deeper Python bindings or a richer UI.

## Python API

After building, a Python extension module is available in `build/python`.

Example:

```python
import sys
sys.path.insert(0, "build/python")

import circuitsim_py

result = circuitsim_py.run_dc("V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n")
print(result["node_voltages"]["out"])

ac_result = circuitsim_py.run_ac("V1 in 0 1\nR1 in out 1k\nC1 out 0 1n\n", [159154.94309189535])
print(ac_result["node_voltages"]["out"][0]["magnitude"])
```

Diagnostics are also exposed through Python:

```python
diagnostics = circuitsim_py.diagnose_netlist("R1 a b 1k\n")
print(diagnostics["has_errors"])
print(diagnostics["messages"])
```

## Parameter Sweeps

Parameterized templates use `{{NAME}}` placeholders.

Example template:

```text
V1 in 0 5
R1 in out 1k
R2 out 0 {{RLOAD}}
```

Run a DC sweep:

```bash
python python/sweep.py dc examples/resistor_divider_param.cir.in RLOAD 1000,2000,4000 --module-dir build/python --observe out --output-json build/dc_sweep.json --plot plots/dc_sweep.png
```

Or with the `Makefile`:

```bash
make sweep-dc
```

This writes:

- a JSON file with all sweep results
- a plot comparing the sweep output

## Diagnostics

CircuitSim now performs a basic topology analysis before solving. The current diagnostics can report:

- missing ground
- floating nodes
- components inside floating regions
- reactive-only networks that are likely to fail in DC analysis

## AC Analysis

CircuitSim now supports frequency-domain small-signal analysis for linear RC circuits.

The current AC solver supports:

- resistors
- capacitors
- voltage sources
- current sources

The current AC solver does not yet support inductors.

## Chip Showcase

CircuitSim includes a simplified on-chip interconnect study that sweeps wire resistance in an RC network and measures the 50% delay at the receiving node.

Run it with:

```bash
python python/chip_showcase.py --module-dir build/python
```

Or with the `Makefile`:

```bash
make showcase
```

This produces:

- `build/chip_showcase.json`
- `plots/chip_showcase_waveforms.png`
- `plots/chip_showcase_delay.png`

The detailed write-up lives in [docs/chip_showcase.md](docs/chip_showcase.md).

## Roadmap

The detailed execution plan lives in [TASKS.md](TASKS.md).
