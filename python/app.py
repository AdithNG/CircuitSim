#!/usr/bin/env python3

from __future__ import annotations

import sys
from pathlib import Path

import matplotlib.pyplot as plt
import streamlit as st

from chip_showcase import crossing_time, parse_values as parse_showcase_values
from sweep import load_module, render_template

ROOT = Path(__file__).resolve().parents[1]
BUILD_PYTHON = ROOT / "build" / "python"


def available_netlists() -> dict[str, Path]:
    return {
        "Resistor divider": ROOT / "examples" / "resistor_divider.cir",
        "RC transient": ROOT / "examples" / "rc_charge.cir",
        "AC low-pass": ROOT / "examples" / "ac_lowpass.cir",
    }


def available_templates() -> dict[str, Path]:
    return {
        "Divider load sweep": ROOT / "examples" / "resistor_divider_param.cir.in",
        "RC resistance sweep": ROOT / "examples" / "rc_resistance_sweep.cir.in",
        "On-chip interconnect": ROOT / "examples" / "on_chip_interconnect.cir.in",
    }


def load_example_text(path: Path) -> str:
    return path.read_text()


def discover_module_dir() -> Path:
    candidates = [
        BUILD_PYTHON,
        ROOT / "build" / "python" / "Debug",
    ]
    for candidate in candidates:
        if not candidate.exists():
            continue
        if any(
            path.name.startswith("circuitsim_py") and path.suffix in {".pyd", ".so", ".dylib"}
            for path in candidate.iterdir()
        ):
            return candidate
    return BUILD_PYTHON


@st.cache_resource
def get_circuitsim_module(module_dir: str):
    return load_module(module_dir)


def plot_waveforms(time_points: list[float], node_series: dict[str, list[float]], title: str):
    figure, axis = plt.subplots(figsize=(8, 4.5))
    for node_name, values in sorted(node_series.items()):
        axis.plot(time_points, values, label=node_name)
    axis.set_title(title)
    axis.set_xlabel("Time (s)")
    axis.set_ylabel("Voltage (V)")
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    axis.legend()
    figure.tight_layout()
    return figure


def plot_ac_magnitude(frequencies: list[float], node_series: dict[str, list[dict]], title: str):
    figure, axis = plt.subplots(figsize=(8, 4.5))
    for node_name, samples in sorted(node_series.items()):
        magnitudes = [sample["magnitude"] for sample in samples]
        axis.semilogx(frequencies, magnitudes, label=node_name)
    axis.set_title(title)
    axis.set_xlabel("Frequency (Hz)")
    axis.set_ylabel("Magnitude")
    axis.grid(True, which="both", linestyle="--", linewidth=0.5, alpha=0.6)
    axis.legend()
    figure.tight_layout()
    return figure


def plot_sweep_curve(x_values: list[float], y_values: list[float], x_label: str, y_label: str, title: str):
    figure, axis = plt.subplots(figsize=(7, 4.2))
    axis.plot(x_values, y_values, marker="o")
    axis.set_title(title)
    axis.set_xlabel(x_label)
    axis.set_ylabel(y_label)
    axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
    figure.tight_layout()
    return figure


def run_chip_showcase(circuitsim_py, template_text: str, wire_values: list[float], time_step: float, stop_time: float):
    results = []
    for wire_resistance in wire_values:
        rendered = render_template(template_text, {"RWIRE": wire_resistance})
        transient = circuitsim_py.run_transient(rendered, time_step, stop_time)
        waveform = transient["node_voltages"]["out"]
        delay = crossing_time(transient["time_points"], waveform, 0.5)
        results.append(
            {
                "wire_resistance": wire_resistance,
                "time_points": transient["time_points"],
                "waveform": waveform,
                "delay_50pct": delay,
                "final_voltage": waveform[-1],
            }
        )
    return results


def format_diagnostic_title(message: dict) -> str:
    code = message["code"].replace("-", " ").replace("_", " ").title()
    return f"{code}"


def render_diagnostic_messages(messages: list[dict]):
    if not messages:
        st.success("No diagnostics to report.")
        return

    for message in messages:
        title = format_diagnostic_title(message)
        body = message["message"]
        severity = message["severity"]

        if severity == "error":
            st.error(f"{title}: {body}")
        else:
            st.warning(f"{title}: {body}")

    with st.expander("Raw diagnostic data"):
        st.json(messages)


def guidance_for_messages(messages: list[dict]) -> list[str]:
    suggestions: list[str] = []
    codes = {message["code"] for message in messages}
    if "missing-ground" in codes:
        suggestions.append("Add a reference node named `0` or `GND` so the circuit has a valid ground.")
    if "floating-nodes" in codes:
        suggestions.append("Connect isolated nodes back to the grounded part of the circuit or remove the floating subnetwork.")
    if "reactive-only-network" in codes:
        suggestions.append("Add a DC path such as a resistor if you want DC operating point analysis to succeed.")
    return suggestions


def render_sidebar() -> tuple[object, str]:
    st.sidebar.title("CircuitSim")
    module_dir = str(discover_module_dir())
    if module_dir not in sys.path:
        sys.path.insert(0, module_dir)
    circuitsim_py = get_circuitsim_module(module_dir)
    st.sidebar.caption("Backed by the C++ simulation core via pybind11.")
    with st.sidebar.expander("Advanced"):
        st.code(module_dir)
    return circuitsim_py, module_dir


def render_dc_tab(circuitsim_py):
    st.subheader("DC Analysis")
    example_name = st.selectbox("DC example", list(available_netlists().keys()), key="dc_example")
    netlist = st.text_area("Netlist", load_example_text(available_netlists()[example_name]), height=160, key="dc_text")
    if st.button("Run DC", key="run_dc"):
        result = circuitsim_py.run_dc(netlist)
        if result["diagnostics"]:
            render_diagnostic_messages(result["diagnostics"])
        st.json(result)


def render_transient_tab(circuitsim_py):
    st.subheader("Transient Analysis")
    default_text = load_example_text(ROOT / "examples" / "rc_charge.cir")
    netlist = st.text_area("Netlist", default_text, height=160, key="tran_text")
    time_step = st.number_input("Time step", value=1e-4, format="%.6e")
    stop_time = st.number_input("Stop time", value=5e-3, format="%.6e")
    if st.button("Run transient", key="run_tran"):
        result = circuitsim_py.run_transient(netlist, time_step, stop_time)
        if result["diagnostics"]:
            render_diagnostic_messages(result["diagnostics"])
        figure = plot_waveforms(result["time_points"], result["node_voltages"], "Transient Response")
        st.pyplot(figure)
        st.json({"time_points": result["time_points"][:5], "node_voltages_preview": {k: v[:5] for k, v in result["node_voltages"].items()}})


def render_ac_tab(circuitsim_py):
    st.subheader("AC Analysis")
    default_text = load_example_text(ROOT / "examples" / "ac_lowpass.cir")
    netlist = st.text_area("Netlist", default_text, height=160, key="ac_text")
    frequency_text = st.text_input("Frequencies (comma-separated Hz)", "1e3,1e4,1e5,1.5915494309189535e5,1e6")
    if st.button("Run AC", key="run_ac"):
        frequencies = [float(value.strip()) for value in frequency_text.split(",") if value.strip()]
        result = circuitsim_py.run_ac(netlist, frequencies)
        if result["diagnostics"]:
            render_diagnostic_messages(result["diagnostics"])
        figure = plot_ac_magnitude(result["frequencies_hz"], result["node_voltages"], "AC Magnitude Response")
        st.pyplot(figure)
        st.json(result)


def render_sweep_tab(circuitsim_py):
    st.subheader("Parameter Sweep")
    template_name = st.selectbox("Template", list(available_templates().keys()), key="sweep_template")
    template_text = st.text_area("Template", load_example_text(available_templates()[template_name]), height=160, key="sweep_text")
    parameter_name = st.text_input("Parameter name", "RLOAD")
    values_text = st.text_input("Values", "1000,2000,4000")
    observe_node = st.text_input("Observed node", "out")
    mode = st.radio("Sweep mode", ["dc", "tran"], horizontal=True)
    time_step = st.number_input("Sweep time step", value=1e-4, format="%.6e")
    stop_time = st.number_input("Sweep stop time", value=5e-3, format="%.6e")
    if st.button("Run sweep", key="run_sweep"):
        values = [float(value.strip()) for value in values_text.split(",") if value.strip()]
        if mode == "dc":
            results = []
            for value in values:
                rendered = render_template(template_text, {parameter_name: value})
                dc_result = circuitsim_py.run_dc(rendered)
                results.append({"parameter": value, "node_voltage": dc_result["node_voltages"][observe_node]})
            figure = plot_sweep_curve(
                values,
                [item["node_voltage"] for item in results],
                parameter_name,
                f"{observe_node} voltage (V)",
                "DC Sweep",
            )
            st.pyplot(figure)
            st.json(results)
        else:
            results = []
            figure, axis = plt.subplots(figsize=(8, 4.5))
            for value in values:
                rendered = render_template(template_text, {parameter_name: value})
                transient_result = circuitsim_py.run_transient(rendered, time_step, stop_time)
                waveform = transient_result["node_voltages"][observe_node]
                axis.plot(transient_result["time_points"], waveform, label=f"{parameter_name}={value:.6g}")
                results.append({"parameter": value, "final_voltage": waveform[-1]})
            axis.set_title("Transient Sweep")
            axis.set_xlabel("Time (s)")
            axis.set_ylabel(f"{observe_node} voltage (V)")
            axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
            axis.legend()
            figure.tight_layout()
            st.pyplot(figure)
            st.json(results)


def render_diagnostics_tab(circuitsim_py):
    st.subheader("Diagnostics")
    netlist = st.text_area("Netlist", "R1 a b 1k\n", height=160, key="diag_text")
    if st.button("Analyze netlist", key="run_diag"):
        result = circuitsim_py.diagnose_netlist(netlist)
        if result["has_errors"]:
            st.error("Diagnostics found blocking issues.")
        else:
            st.success("No blocking issues found.")
        render_diagnostic_messages(result["messages"])
        suggestions = guidance_for_messages(result["messages"])
        if suggestions:
            st.info("Suggested next steps:")
            for suggestion in suggestions:
                st.markdown(f"- {suggestion}")


def render_showcase_tab(circuitsim_py):
    st.subheader("Chip Showcase")
    template_text = st.text_area(
        "Interconnect template",
        load_example_text(ROOT / "examples" / "on_chip_interconnect.cir.in"),
        height=160,
        key="showcase_text",
    )
    wire_values = st.text_input("Wire resistances (ohm)", "50,100,200,400")
    time_step = st.number_input("Showcase time step", value=1e-11, format="%.6e")
    stop_time = st.number_input("Showcase stop time", value=6e-9, format="%.6e")
    if st.button("Run showcase", key="run_showcase"):
        results = run_chip_showcase(
            circuitsim_py,
            template_text,
            parse_showcase_values(wire_values),
            time_step,
            stop_time,
        )
        waveform_figure, waveform_axis = plt.subplots(figsize=(8, 4.5))
        for item in results:
            waveform_axis.plot(item["time_points"], item["waveform"], label=f"RWIRE={item['wire_resistance']:.0f}")
        waveform_axis.set_title("On-Chip Interconnect Waveforms")
        waveform_axis.set_xlabel("Time (s)")
        waveform_axis.set_ylabel("Receiver voltage (V)")
        waveform_axis.grid(True, linestyle="--", linewidth=0.5, alpha=0.6)
        waveform_axis.legend()
        waveform_figure.tight_layout()
        st.pyplot(waveform_figure)

        delay_figure = plot_sweep_curve(
            [item["wire_resistance"] for item in results],
            [item["delay_50pct"] * 1e12 for item in results],
            "Wire resistance (ohm)",
            "50% delay (ps)",
            "Interconnect Delay vs Resistance",
        )
        st.pyplot(delay_figure)
        st.json(results)


def main():
    st.set_page_config(page_title="CircuitSim UI", layout="wide")
    st.title("CircuitSim Workbench")
    st.caption("Interactive front-end for the C++ simulation core, Python bindings, and workflow tooling.")

    circuitsim_py, _module_dir = render_sidebar()

    tabs = st.tabs(["DC", "Transient", "AC", "Sweep", "Diagnostics", "Chip Showcase"])
    with tabs[0]:
        render_dc_tab(circuitsim_py)
    with tabs[1]:
        render_transient_tab(circuitsim_py)
    with tabs[2]:
        render_ac_tab(circuitsim_py)
    with tabs[3]:
        render_sweep_tab(circuitsim_py)
    with tabs[4]:
        render_diagnostics_tab(circuitsim_py)
    with tabs[5]:
        render_showcase_tab(circuitsim_py)


if __name__ == "__main__":
    main()
