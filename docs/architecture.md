# Architecture Notes

CircuitSim is being built in layers:

1. Netlist parser and validation
2. Circuit graph and core data structures
3. DC solver
4. Transient solver
5. Python automation layer
6. Workflow tooling

The current codebase focuses on keeping the parser logic small, explicit, and well tested so that solver work has a reliable foundation.
