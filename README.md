# CircuitSim

CircuitSim is a software-first circuit simulation and design exploration platform for engineers working on chip-adjacent problems. The goal is to build something that is not just a basic circuit solver, but a small EDA-style workflow system: users can define circuits, simulate behavior, run parameter sweeps, diagnose failures, and explore tradeoffs through a Python automation layer.

This project is designed to show strength in the kinds of problems that companies like Keysight, NVIDIA, and Qualcomm care about:

- numerical methods and simulation
- graph and netlist modeling
- performance-conscious systems programming
- Python tooling and workflow automation
- developer-facing product design for technical users

## What The Project Does

CircuitSim will simulate real circuits rather than mock them.

Core capabilities:

- Parse a circuit netlist into an internal graph representation
- Run DC operating point analysis
- Run transient simulation over time
- Run AC small-signal frequency sweeps
- Support basic components such as resistors, capacitors, inductors, voltage sources, and current sources
- Support reusable subcircuits
- Visualize waveforms and compare simulation runs

On top of the simulator, CircuitSim adds a workflow layer inspired by EDA tools:

- Parameter sweeps for design space exploration
- Python scripting for automation and optimization
- Design-flow diagnostics for common setup and convergence issues
- Run comparison tools to answer "what changed and why?"
- Scenario management for different circuit configurations

## Project Vision

The interesting part of this project is not only solving circuits. It is building a tool that helps users move from:

1. writing a circuit
2. running a simulation
3. debugging a bad result
4. sweeping parameters
5. identifying a better design

That is why this project is more than a toy SPICE clone. It is meant to feel like a lightweight EDA workflow tool.

## Example Use Case

One target use case is early exploration of chip-related electrical behavior without requiring hardware. For example, a user could model a simplified on-chip interconnect or power delivery network as an RC or RLC circuit and then:

- measure delay and settling time
- observe ringing or voltage droop
- study sensitivity to parasitics
- compare multiple design choices
- automate optimization from Python

This keeps the project relevant to semiconductor software roles while staying fully accessible without lab experience.

## Why This Is A Strong Portfolio Project

This project demonstrates several valuable engineering skills in one system:

- building a parser and intermediate representation
- implementing linear algebra and numerical simulation logic
- designing extensible software architecture
- exposing low-level functionality through a high-level Python API
- creating usable tooling for engineers, not just algorithms

It also creates strong interview discussion topics:

- solver architecture and tradeoffs
- convergence and numerical stability
- performance bottlenecks
- user experience for technical workflows
- how to design software for experimentation and debugging

## Planned Architecture

### Simulation Core

A performance-focused core, likely in C++ or another systems-friendly language, will handle:

- netlist parsing
- matrix assembly
- circuit stamping
- numerical solving
- transient stepping

### Automation Layer

A Python layer will make the simulator easy to script for:

- batch runs
- parameter sweeps
- optimization loops
- dataset generation
- analysis notebooks

### Workflow Layer

A higher-level product layer will help users work more effectively:

- input validation
- diagnostics and explanations
- reusable experiment definitions
- result comparison
- plotting and reporting

## Stretch Features

If time allows, the most compelling advanced additions would be:

- model fitting from measured CSV data
- simple parasitic-aware interconnect modeling
- optimization-guided parameter search
- Monte Carlo variation analysis
- interactive circuit editor or waveform dashboard

## Development Priorities

Recommended build order:

1. Netlist parser and internal circuit graph
2. DC solver
3. Transient simulation
4. Python API
5. Parameter sweep framework
6. Diagnostics and workflow tooling
7. Visualization and polish

## Relationship To "CircuitSim Flow"

Yes, this is the same core idea as the earlier "CircuitSim Flow" concept, but refined.

"CircuitSim Flow" was the broader pitch: a mini EDA workflow platform built around simulation, automation, and user design flows.

This README describes the concrete version of that idea:

- the foundation is a real circuit simulator
- the differentiator is the workflow and automation layer on top

So the simplest way to think about it is:

CircuitSim Flow = the product vision  
CircuitSim = the implementation of that vision as a simulation-first project

## Current Status

This repository is currently in the planning and architecture stage.

The next milestone is to implement a minimal end-to-end path:

- parse a small netlist
- run a DC solve
- simulate a simple RC transient
- expose one scripted parameter sweep from Python

Once that works, the project can grow into the workflow and tooling features that make it stand out.
