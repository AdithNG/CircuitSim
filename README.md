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
- a small CLI for running DC or transient analysis on a netlist file
- automated tests and CI

## Repository Layout

```text
CircuitSim/
  README.md
  TASKS.md
  CMakeLists.txt
  include/
  src/
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
cmake -S . -B build
cmake --build build --config Debug
ctest --test-dir build -C Debug --output-on-failure
```

## Run The CLI

```bash
./build/Debug/circuitsim_cli examples/resistor_divider.cir
```

Transient example:

```bash
./build/Debug/circuitsim_cli tran examples/rc_charge.cir 1e-4 5e-3
```

## Roadmap

The detailed execution plan lives in [TASKS.md](TASKS.md).
