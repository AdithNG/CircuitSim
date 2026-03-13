# Chip Showcase

This showcase frames CircuitSim as an early design-exploration tool for simplified on-chip interconnect behavior.

## Scenario

We model a driver resistance, a wire resistance, and a capacitive load:

- `RDRV`: output resistance of the upstream driver
- `RWIRE`: resistance of the interconnect segment
- `CLOAD`: downstream load capacitance

The example uses a transient step response to answer a common chip-design question:

How does increasing wire resistance affect receiver settling and propagation delay?

## What The Script Produces

`python/chip_showcase.py` runs a resistance sweep over `RWIRE` and produces:

- a JSON result file with the full waveform data
- a waveform comparison plot
- a delay-vs-resistance plot

The measured metric is the 50% threshold crossing time at the receiver node.

## Why This Matters

Even though this is a simplified RC model, it demonstrates the type of tradeoff exploration that appears in EDA and semiconductor tooling:

- signal delay rises as wire resistance increases
- waveform shape changes as the RC time constant grows
- engineers can use scripted sweeps to compare design choices quickly

This is exactly the kind of software workflow that makes CircuitSim more than just a solver.
