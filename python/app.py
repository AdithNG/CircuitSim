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
PLOT_COLORS = ["#0f766e", "#f97316", "#2563eb", "#be123c", "#7c3aed", "#65a30d"]


def apply_theme():
    st.markdown(
        """
        <style>
        @import url('https://fonts.googleapis.com/css2?family=Space+Grotesk:wght@500;700&family=Source+Sans+3:wght@400;600&display=swap');

        :root {
            --cs-ink: #14213d;
            --cs-muted: #52607a;
            --cs-panel: rgba(255, 255, 255, 0.94);
            --cs-line: rgba(15, 23, 42, 0.10);
            --cs-accent: #0f766e;
            --cs-accent-soft: rgba(15, 118, 110, 0.10);
            --cs-bg: #f6f8fb;
            --cs-bg-soft: #eef4f6;
        }

        .stApp {
            background: linear-gradient(180deg, var(--cs-bg), var(--cs-bg-soft));
        }

        header[data-testid="stHeader"] {
            display: none;
        }

        [data-testid="stToolbar"] {
            display: none;
        }

        [data-testid="stDecoration"] {
            display: none;
        }

        [data-testid="stStatusWidget"] {
            display: none;
        }

        #MainMenu {
            visibility: hidden;
        }

        footer {
            visibility: hidden;
        }

        html, body, [class*="css"] {
            font-family: "Source Sans 3", "Segoe UI", sans-serif;
            color: var(--cs-ink);
        }

        h1, h2, h3, .hero-title {
            font-family: "Space Grotesk", "Avenir Next", sans-serif;
            letter-spacing: -0.02em;
            color: var(--cs-ink);
        }

        .block-container {
            padding-top: 1.4rem;
            padding-bottom: 2rem;
            max-width: 1180px;
        }

        [data-testid="stSidebar"] {
            display: none;
        }

        [data-testid="collapsedControl"] {
            display: none;
        }

        .hero-shell {
            padding: 1rem 1.15rem;
            border: 1px solid var(--cs-line);
            border-radius: 1rem;
            background: var(--cs-panel);
            box-shadow: 0 10px 28px rgba(20, 33, 61, 0.05);
            margin-bottom: 0.9rem;
        }

        .hero-kicker {
            text-transform: uppercase;
            letter-spacing: 0.16em;
            font-size: 0.72rem;
            font-weight: 700;
            color: var(--cs-accent);
        }

        .hero-title {
            font-size: 1.95rem;
            line-height: 1.08;
            margin: 0.2rem 0 0.35rem 0;
        }

        .hero-copy {
            max-width: 48rem;
            color: var(--cs-muted);
            font-size: 0.98rem;
            line-height: 1.45;
            margin-bottom: 0.65rem;
        }

        .hero-pills {
            display: flex;
            gap: 0.55rem;
            flex-wrap: wrap;
        }

        .hero-pill {
            background: var(--cs-accent-soft);
            border: 1px solid rgba(15, 118, 110, 0.18);
            color: var(--cs-accent);
            border-radius: 999px;
            padding: 0.24rem 0.62rem;
            font-size: 0.8rem;
            font-weight: 600;
        }

        .section-lead {
            color: var(--cs-muted);
            margin-top: -0.2rem;
            margin-bottom: 0.9rem;
        }

        .diag-card {
            border: 1px solid var(--cs-line);
            border-left-width: 5px;
            border-radius: 0.85rem;
            padding: 0.68rem 0.82rem;
            margin-bottom: 0.55rem;
            background: rgba(255, 255, 255, 0.96);
        }

        .diag-card.error {
            border-left-color: #be123c;
        }

        .diag-card.warning {
            border-left-color: #d97706;
        }

        .diag-title {
            font-weight: 700;
            margin-bottom: 0.1rem;
        }

        .diag-body {
            color: var(--cs-muted);
        }

        [data-baseweb="tab-list"] {
            gap: 0.35rem;
        }

        [data-baseweb="tab"] {
            border-radius: 0.9rem 0.9rem 0 0;
            padding: 0.42rem 0.82rem;
            background: rgba(255,255,255,0.82);
            border: 1px solid rgba(15, 23, 42, 0.08);
        }

        [aria-selected="true"][data-baseweb="tab"] {
            background: rgba(255,255,255,0.98);
            border-bottom-color: rgba(255,255,255,0.98);
            color: var(--cs-accent);
        }

        button[kind="primary"] {
            border-radius: 0.8rem;
        }

        [data-testid="stMetric"] {
            background: rgba(255,255,255,0.96);
            border: 1px solid var(--cs-line);
            border-radius: 0.9rem;
            padding: 0.6rem 0.75rem;
            box-shadow: 0 8px 20px rgba(20, 33, 61, 0.04);
        }

        [data-testid="stDataFrame"], [data-testid="stExpander"] {
            background: rgba(255,255,255,0.96);
            border-radius: 0.9rem;
        }

        .stTextArea textarea, .stTextInput input, .stNumberInput input {
            background: rgba(255,255,255,0.96);
        }

        @media (max-width: 900px) {
            .hero-title {
                font-size: 1.5rem;
            }

            .block-container {
                padding-top: 1rem;
            }
        }
        </style>
        """,
        unsafe_allow_html=True,
    )


def apply_plot_style(axis):
    axis.set_facecolor("#fffdf9")
    axis.grid(True, linestyle="--", linewidth=0.7, alpha=0.35, color="#64748b")
    for spine in axis.spines.values():
        spine.set_color("#cbd5e1")
    axis.tick_params(colors="#334155")


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
    for index, (node_name, values) in enumerate(sorted(node_series.items())):
        color = PLOT_COLORS[index % len(PLOT_COLORS)]
        axis.plot(time_points, values, label=node_name)
        axis.lines[-1].set_color(color)
    axis.set_title(title)
    axis.set_xlabel("Time (s)")
    axis.set_ylabel("Voltage (V)")
    apply_plot_style(axis)
    axis.legend()
    figure.tight_layout()
    return figure


def plot_ac_magnitude(frequencies: list[float], node_series: dict[str, list[dict]], title: str):
    figure, axis = plt.subplots(figsize=(8, 4.5))
    for index, (node_name, samples) in enumerate(sorted(node_series.items())):
        color = PLOT_COLORS[index % len(PLOT_COLORS)]
        magnitudes = [sample["magnitude"] for sample in samples]
        axis.semilogx(frequencies, magnitudes, label=node_name, color=color, linewidth=2.2)
    axis.set_title(title)
    axis.set_xlabel("Frequency (Hz)")
    axis.set_ylabel("Magnitude")
    apply_plot_style(axis)
    axis.legend()
    figure.tight_layout()
    return figure


def plot_ac_phase(frequencies: list[float], node_series: dict[str, list[dict]], title: str):
    figure, axis = plt.subplots(figsize=(8, 4.5))
    for index, (node_name, samples) in enumerate(sorted(node_series.items())):
        color = PLOT_COLORS[index % len(PLOT_COLORS)]
        phases = [sample["phase_rad"] for sample in samples]
        axis.semilogx(frequencies, phases, label=node_name, color=color, linewidth=2.2)
    axis.set_title(title)
    axis.set_xlabel("Frequency (Hz)")
    axis.set_ylabel("Phase (rad)")
    apply_plot_style(axis)
    axis.legend()
    figure.tight_layout()
    return figure


def plot_sweep_curve(x_values: list[float], y_values: list[float], x_label: str, y_label: str, title: str):
    figure, axis = plt.subplots(figsize=(7, 4.2))
    axis.plot(x_values, y_values, marker="o", color=PLOT_COLORS[0], linewidth=2.2, markersize=7)
    axis.set_title(title)
    axis.set_xlabel(x_label)
    axis.set_ylabel(y_label)
    apply_plot_style(axis)
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
                "summary": transient["summary"],
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


def render_intro(title: str, description: str, pills: list[str] | None = None):
    pill_markup = ""
    if pills:
        pill_markup = '<div class="hero-pills">' + "".join(
            f'<span class="hero-pill">{pill}</span>' for pill in pills
        ) + "</div>"
    st.markdown(
        f"""
        <div class="hero-shell">
            <div class="hero-kicker">CircuitSim</div>
            <div class="hero-title">{title}</div>
            <div class="hero-copy">{description}</div>
            {pill_markup}
        </div>
        """,
        unsafe_allow_html=True,
    )


def render_diagnostic_messages(messages: list[dict]):
    if not messages:
        st.success("No diagnostics to report.")
        return

    errors = [message for message in messages if message["severity"] == "error"]
    warnings = [message for message in messages if message["severity"] != "error"]

    if errors:
        st.error(f"{len(errors)} blocking issue(s) found.")
    elif warnings:
        st.warning(f"{len(warnings)} warning(s) found.")

    st.markdown("**Issues**")
    for message in messages:
        title = format_diagnostic_title(message)
        severity = message["severity"]
        st.markdown(
            f"""
            <div class="diag-card {severity}">
                <div class="diag-title">{title}</div>
                <div class="diag-body">{message['message']}</div>
            </div>
            """,
            unsafe_allow_html=True,
        )

    with st.expander("Raw diagnostic data"):
        st.json(messages)


def guidance_for_messages(messages: list[dict]) -> list[str]:
    suggestions: list[str] = []
    codes = {message["code"] for message in messages}
    if "floating-nodes" in codes:
        suggestions.append("Connect isolated nodes back to the grounded part of the circuit or remove the floating subnetwork.")
    if "reactive-only-network" in codes:
        suggestions.append("Add a DC path such as a resistor if you want DC operating point analysis to succeed.")
    return suggestions


def render_summary(summary: dict):
    st.markdown("**Run Summary**")
    metrics = st.columns(4)
    metrics[0].metric("Analysis", summary["analysis_type"].title())
    metrics[1].metric("Nodes", int(summary["node_count"]))
    metrics[2].metric("Components", int(summary["component_count"]))
    metrics[3].metric("Samples", int(summary["sample_count"]))

    detail_columns = st.columns(5)
    detail_columns[0].metric("Resistors", int(summary["resistor_count"]))
    detail_columns[1].metric("Capacitors", int(summary["capacitor_count"]))
    detail_columns[2].metric("Inductors", int(summary["inductor_count"]))
    detail_columns[3].metric("V Sources", int(summary["voltage_source_count"]))
    detail_columns[4].metric("I Sources", int(summary["current_source_count"]))

    if summary["analysis_type"] == "transient":
        transient_columns = st.columns(2)
        transient_columns[0].metric("Time Step", f"{summary['time_step']:.3e} s")
        transient_columns[1].metric("Stop Time", f"{summary['stop_time']:.3e} s")


def render_scalar_results(title: str, values: dict[str, float], unit: str):
    if not values:
        return
    st.markdown(f"**{title}**")
    rows = [{"name": name, "value": value, "unit": unit} for name, value in sorted(values.items())]
    st.dataframe(rows, width="stretch", hide_index=True)


def render_results_table(title: str, rows: list[dict]):
    if not rows:
        return
    st.markdown(f"**{title}**")
    st.dataframe(rows, width="stretch", hide_index=True)


def render_series_preview(title: str, x_label: str, x_values: list[float], series: dict[str, list[float]]):
    st.markdown(f"**{title}**")
    preview_count = min(5, len(x_values))
    rows = []
    for index in range(preview_count):
        row = {x_label: x_values[index]}
        for node_name, values in sorted(series.items()):
            row[node_name] = values[index]
        rows.append(row)
    st.dataframe(rows, width="stretch", hide_index=True)


def render_complex_series_preview(
    title: str,
    frequencies: list[float],
    node_series: dict[str, list[dict]],
):
    st.markdown(f"**{title}**")
    rows = []
    for index, frequency in enumerate(frequencies):
        for node_name, samples in sorted(node_series.items()):
            sample = samples[index]
            rows.append(
                {
                    "frequency_hz": frequency,
                    "node": node_name,
                    "magnitude": sample["magnitude"],
                    "phase_rad": sample["phase_rad"],
                }
            )
    st.dataframe(rows, width="stretch", hide_index=True)


def render_dc_tab(circuitsim_py):
    st.subheader("DC Analysis")
    st.caption("Inspect steady-state node voltages and source currents for grounded linear circuits.")
    example_name = st.selectbox("DC example", list(available_netlists().keys()), key="dc_example")
    netlist = st.text_area("Netlist", load_example_text(available_netlists()[example_name]), height=160, key="dc_text")
    if st.button("Run DC", key="run_dc"):
        result = circuitsim_py.run_dc(netlist)
        if result["diagnostics"]:
            render_diagnostic_messages(result["diagnostics"])
        render_summary(result["summary"])
        value_col, current_col = st.columns(2)
        with value_col:
            render_scalar_results("Node Voltages", result["node_voltages"], "V")
        with current_col:
            render_scalar_results("Source Currents", result["source_currents"], "A")
        with st.expander("Raw result data"):
            st.json(result)


def render_transient_tab(circuitsim_py):
    st.subheader("Transient Analysis")
    st.caption("Follow voltages and currents over time for RC and RL responses.")
    default_text = load_example_text(ROOT / "examples" / "rc_charge.cir")
    netlist = st.text_area("Netlist", default_text, height=160, key="tran_text")
    time_step = st.number_input("Time step", value=1e-4, format="%.6e")
    stop_time = st.number_input("Stop time", value=5e-3, format="%.6e")
    if st.button("Run transient", key="run_tran"):
        result = circuitsim_py.run_transient(netlist, time_step, stop_time)
        if result["diagnostics"]:
            render_diagnostic_messages(result["diagnostics"])
        render_summary(result["summary"])
        figure = plot_waveforms(result["time_points"], result["node_voltages"], "Transient Response")
        st.pyplot(figure)
        preview_col, current_col = st.columns(2)
        with preview_col:
            render_series_preview("Waveform Preview", "time_s", result["time_points"], result["node_voltages"])
        with current_col:
            render_series_preview(
                "Source Current Preview",
                "time_s",
                result["time_points"],
                result["source_currents"],
            )
        with st.expander("Raw result data"):
            st.json(result)


def render_ac_tab(circuitsim_py):
    st.subheader("AC Analysis")
    st.caption("Study frequency response with both magnitude and phase in one place.")
    default_text = load_example_text(ROOT / "examples" / "ac_lowpass.cir")
    netlist = st.text_area("Netlist", default_text, height=160, key="ac_text")
    frequency_text = st.text_input("Frequencies (comma-separated Hz)", "1e3,1e4,1e5,1.5915494309189535e5,1e6")
    if st.button("Run AC", key="run_ac"):
        frequencies = [float(value.strip()) for value in frequency_text.split(",") if value.strip()]
        result = circuitsim_py.run_ac(netlist, frequencies)
        if result["diagnostics"]:
            render_diagnostic_messages(result["diagnostics"])
        render_summary(result["summary"])
        figure_columns = st.columns(2)
        with figure_columns[0]:
            figure = plot_ac_magnitude(result["frequencies_hz"], result["node_voltages"], "AC Magnitude Response")
            st.pyplot(figure)
        with figure_columns[1]:
            phase_figure = plot_ac_phase(result["frequencies_hz"], result["node_voltages"], "AC Phase Response")
            st.pyplot(phase_figure)
        preview_col, current_col = st.columns(2)
        with preview_col:
            render_complex_series_preview("Voltage Response Table", result["frequencies_hz"], result["node_voltages"])
        with current_col:
            render_complex_series_preview("Source Current Table", result["frequencies_hz"], result["source_currents"])
        with st.expander("Raw result data"):
            st.json(result)


def render_sweep_tab(circuitsim_py):
    st.subheader("Parameter Sweep")
    st.caption("Run small design-space experiments by varying one parameter across multiple simulations.")
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
            summaries = []
            for value in values:
                rendered = render_template(template_text, {parameter_name: value})
                dc_result = circuitsim_py.run_dc(rendered)
                summaries.append(dc_result["summary"])
                results.append({"parameter": value, "node_voltage": dc_result["node_voltages"][observe_node]})
            figure = plot_sweep_curve(
                values,
                [item["node_voltage"] for item in results],
                parameter_name,
                f"{observe_node} voltage (V)",
                "DC Sweep",
            )
            st.pyplot(figure)
            if summaries:
                render_summary(summaries[0])
            render_results_table("Sweep Results", results)
        else:
            results = []
            summaries = []
            figure, axis = plt.subplots(figsize=(8, 4.5))
            for value in values:
                rendered = render_template(template_text, {parameter_name: value})
                transient_result = circuitsim_py.run_transient(rendered, time_step, stop_time)
                summaries.append(transient_result["summary"])
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
            if summaries:
                render_summary(summaries[0])
            render_results_table("Sweep Results", results)


def render_diagnostics_tab(circuitsim_py):
    st.subheader("Diagnostics")
    st.caption("Catch missing ground references, floating subgraphs, and other analysis blockers early.")
    if "diag_text" not in st.session_state:
        st.session_state["diag_text"] = "V1 a 0 5\nR1 a b 1k\nR2 b 0 2k\n"

    action_left, action_right = st.columns(2)
    with action_left:
        if st.button("Load valid example", key="load_diag_valid"):
            st.session_state["diag_text"] = "V1 a 0 5\nR1 a b 1k\nR2 b 0 2k\n"
    with action_right:
        if st.button("Load broken example", key="load_diag_invalid"):
            st.session_state["diag_text"] = "R1 a b 1k\n"

    netlist = st.text_area("Netlist", height=160, key="diag_text")
    if st.button("Analyze netlist", key="run_diag"):
        result = circuitsim_py.diagnose_netlist(netlist)
        if result["has_errors"]:
            st.caption("Review the issues below and update the netlist before running analyses.")
        else:
            st.success("No blocking issues found.")
        render_diagnostic_messages(result["messages"])
        suggestions = guidance_for_messages(result["messages"])
        if suggestions:
            st.markdown("**Suggested next steps**")
            for suggestion in suggestions:
                st.markdown(f"- {suggestion}")


def render_showcase_tab(circuitsim_py):
    st.subheader("Chip Showcase")
    st.caption("Explore how wire resistance changes delay in a simplified on-chip interconnect model.")
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
        if results:
            render_summary(results[0]["summary"])
        st.pyplot(waveform_figure)

        delay_figure = plot_sweep_curve(
            [item["wire_resistance"] for item in results],
            [item["delay_50pct"] * 1e12 for item in results],
            "Wire resistance (ohm)",
            "50% delay (ps)",
            "Interconnect Delay vs Resistance",
        )
        st.pyplot(delay_figure)
        render_results_table(
            "Showcase Results",
            [
                {
                    "wire_resistance_ohm": item["wire_resistance"],
                    "delay_50pct_ps": item["delay_50pct"] * 1e12,
                    "final_voltage_v": item["final_voltage"],
                }
                for item in results
            ],
        )


def main():
    st.set_page_config(page_title="CircuitSim UI", layout="wide", initial_sidebar_state="collapsed")
    apply_theme()
    render_intro(
        "Circuit Simulation That Feels Like A Workbench",
        "CircuitSim combines a C++ solver core with Python automation and a visual Streamlit front-end for exploring design behavior, debugging netlists, and running small EDA-style experiments.",
        ["C++ Core", "Python Automation", "DC / Transient / AC", "EDA Workflow"],
    )

    module_dir = str(discover_module_dir())
    if module_dir not in sys.path:
        sys.path.insert(0, module_dir)
    circuitsim_py = get_circuitsim_module(module_dir)

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
