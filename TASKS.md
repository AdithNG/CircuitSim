# CircuitSim Task Plan

This document turns the project vision in `README.md` into a practical build plan. The goal is to ship CircuitSim in small, verifiable slices that each improve the portfolio story and make sense as standalone commits.

## Project Goal

Build a simulation-first circuit design exploration tool that demonstrates:

- strong software fundamentals
- numerical and systems thinking
- Python and C++ interoperability
- workflow design for technical users

The minimum compelling version is not "a complete SPICE replacement." It is:

- a real circuit simulator
- with a clean internal architecture
- a scriptable automation layer
- and a few workflow features that make engineering experimentation easier

## Recommended Tech Direction

Start with the simplest architecture that still supports the long-term vision:

- Core language: C++
- Build system: CMake
- Tests: Catch2 or GoogleTest
- Python bindings: `pybind11`
- Plotting and examples: Python scripts or notebooks
- File format: simple SPICE-like netlists

This direction keeps the project relevant to Keysight-style EDA work while also looking strong for systems and semiconductor software roles.

## Core Principles

- Keep the first milestones small and testable.
- Prefer correctness and debuggability before performance tuning.
- Build clean internal abstractions before adding UI polish.
- Treat Python automation as a first-class feature, not an afterthought.
- Every milestone should produce something demoable.

## Milestone Plan

## Milestone 0: Repository Setup

Goal:
Create a clean base repository that is ready for iterative development.

Tasks:

- Add initial source tree layout
- Add `CMakeLists.txt`
- Add test target
- Add formatting and linting plan
- Add `.gitignore`
- Add sample netlist directory
- Add `docs/` directory for technical notes

Deliverable:
A repo that builds, runs tests, and is ready for solver implementation.

Exit criteria:

- project builds locally
- test framework runs successfully
- at least one placeholder test passes

## Milestone 1: Netlist Parser And Circuit Graph

Goal:
Parse a small SPICE-like input format into a validated internal representation.

Tasks:

- Define supported netlist grammar for initial version
- Implement tokenizer or line-based parser
- Parse components:
  - resistor
  - capacitor
  - inductor
  - voltage source
  - current source
- Represent nodes and components in a circuit graph
- Add validation for:
  - unknown component types
  - malformed values
  - missing node references
  - duplicate identifiers

Deliverable:
A parser that reads a netlist file and prints or exposes a validated in-memory circuit model.

Exit criteria:

- valid netlists parse successfully
- invalid netlists produce helpful errors
- parser unit tests cover happy-path and failure cases

## Milestone 2: DC Solver

Goal:
Compute DC operating points for linear circuits.

Tasks:

- Implement Modified Nodal Analysis matrix assembly
- Add stamping logic for:
  - resistor
  - current source
  - voltage source
- Add linear system solver
- Validate results against hand-worked examples
- Add output formatting for node voltages and source currents

Deliverable:
Command-line or test-driven DC solve for simple circuits.

Exit criteria:

- resistor-divider example matches expected values
- current-source example matches expected values
- source current reporting works
- solver tests cover at least 3 reference circuits

## Milestone 3: Transient Simulation

Goal:
Simulate time-domain behavior for circuits with energy storage.

Tasks:

- Add transient stepping loop
- Add capacitor and inductor stamping for time-domain simulation
- Start with backward Euler for stability
- Add configurable timestep and end time
- Support waveform output for selected nodes
- Validate against RC charging and RL response examples

Deliverable:
Transient simulation for simple RC and RL circuits with plottable output.

Exit criteria:

- RC charging curve behaves correctly
- RL response behaves correctly
- waveform output can be consumed by Python plotting scripts

## Milestone 4: Python Automation Layer

Goal:
Expose simulation functionality through Python so experiments can be scripted.

Tasks:

- Add `pybind11` bindings
- Expose netlist loading
- Expose DC and transient execution
- Return structured results to Python
- Add one example script that runs a simulation and plots output

Deliverable:
A Python API that can load a netlist, run a simulation, and inspect results.

Exit criteria:

- Python script can run a DC solve
- Python script can run a transient simulation
- results are accessible as arrays or structured objects

## Milestone 5: Parameter Sweeps And Experiment Runner

Goal:
Turn the simulator into a design exploration tool.

Tasks:

- Add parameter substitution in netlists
- Implement sweep runner in Python
- Support single-parameter sweeps first
- Save results in a structured format
- Add comparison plotting for multiple runs
- Add one example study:
  - RC delay vs resistance
  - interconnect settling vs capacitance

Deliverable:
A repeatable experiment workflow that compares simulation results across parameter choices.

Exit criteria:

- user can define a parameterized circuit
- sweep script runs multiple cases automatically
- comparison plots are generated from one command

## Milestone 6: Diagnostics And Workflow Features

Goal:
Add the features that make CircuitSim feel like a workflow tool instead of just a solver.

Tasks:

- Detect floating nodes
- Detect unsupported topologies with clear messages
- Report singular matrix failures clearly
- Add basic convergence diagnostics
- Add netlist validation hints before simulation starts
- Add run metadata and simulation summaries

Deliverable:
A simulator that helps users understand why a run failed or what to fix next.

Exit criteria:

- common input mistakes produce helpful explanations
- failure modes are covered by tests
- simulation reports include useful context

## Milestone 7: Chip-Focused Showcase Example

Goal:
Create a polished example that connects the project to semiconductor and EDA roles.

Tasks:

- Build a simplified on-chip interconnect model as an RC or RLC network
- Show tradeoffs such as:
  - delay
  - ringing
  - settling time
  - sensitivity to parasitics
- Add a scripted sweep or optimization study
- Document the example clearly in the README or `docs/`

Deliverable:
A portfolio-ready demo that frames CircuitSim as a chip-design exploration tool.

Exit criteria:

- example is reproducible from the repo
- results are visualized clearly
- write-up explains engineering tradeoffs in plain language

## Milestone 8: Performance And Polish

Goal:
Improve engineering quality once the core workflow is working.

Tasks:

- profile matrix assembly and solve time
- reduce unnecessary allocations
- improve API ergonomics
- clean up error messages
- improve docs and examples
- add CI for build and test automation

Deliverable:
A polished, easier-to-demo version of the project.

Exit criteria:

- basic profiling notes are documented
- CI runs build and tests automatically
- docs are good enough for a fresh user to reproduce examples

## Suggested Commit Strategy

We should commit in small units that preserve a clean project story.

Recommended early commits:

1. bootstrap repo structure, build system, and tests
2. add initial netlist parser and parser tests
3. add circuit graph representation and validation
4. implement DC solver for linear circuits
5. add transient solver for RC example
6. add Python bindings and first plotting script
7. add parameter sweep framework
8. add diagnostics and workflow reporting
9. add chip-focused showcase example
10. polish docs, CI, and performance notes

## Testing Strategy

Every milestone should introduce tests, not just features.

Test categories:

- parser unit tests
- solver correctness tests
- regression tests for known circuits
- failure-mode tests for invalid netlists
- Python integration smoke tests

Reference circuits to include early:

- resistor divider
- parallel resistor network
- current-source driven network
- RC charging circuit
- RL step response

## Recommended Repository Structure

One reasonable starting layout:

```text
CircuitSim/
  CMakeLists.txt
  README.md
  TASKS.md
  docs/
  examples/
  include/
  src/
  tests/
  python/
  data/
```

## Immediate Next Steps

These are the first tasks to tackle once the GitHub repo is ready:

1. Initialize the repository structure and build system.
2. Add a minimal test target and one passing smoke test.
3. Implement the first version of the netlist parser.
4. Add parser tests and error reporting.
5. Define the internal circuit representation before starting the solver.

## Notes On Scope

To keep this project realistic and impressive, avoid these traps early:

- trying to support every SPICE feature
- building a full GUI before the solver works
- over-optimizing before correctness is established
- adding ML features before the core workflow is solid

The strongest first version is:

- a reliable simulator for small circuits
- with a clean Python automation interface
- and one standout workflow feature that makes experimentation easier
