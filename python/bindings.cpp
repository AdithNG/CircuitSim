#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <stdexcept>
#include <string>

#include "circuitsim/ac_solver.h"
#include "circuitsim/dc_solver.h"
#include "circuitsim/diagnostics.h"
#include "circuitsim/netlist_parser.h"
#include "circuitsim/transient_solver.h"

namespace py = pybind11;

namespace {

circuitsim::Circuit parse_or_throw(const std::string& text) {
    const circuitsim::NetlistParser parser;
    const auto result = parser.parse_text(text);
    if (!result.ok()) {
        std::string message = "Netlist parse failed:";
        for (const auto& error : result.errors) {
            message += "\n";
            if (error.line_number != 0) {
                message += "line " + std::to_string(error.line_number) + ": ";
            }
            message += error.message;
        }
        throw std::runtime_error(message);
    }
    return result.circuit;
}

py::dict component_to_dict(const circuitsim::Component& component) {
    py::dict data;
    data["id"] = component.id;
    data["type"] = [&component]() {
        switch (component.type) {
            case circuitsim::ComponentType::resistor:
                return "resistor";
            case circuitsim::ComponentType::capacitor:
                return "capacitor";
            case circuitsim::ComponentType::inductor:
                return "inductor";
            case circuitsim::ComponentType::voltage_source:
                return "voltage_source";
            case circuitsim::ComponentType::current_source:
                return "current_source";
        }
        return "unknown";
    }();
    data["node_positive"] = component.node_positive;
    data["node_negative"] = component.node_negative;
    data["value"] = component.value;
    data["line_number"] = component.line_number;
    return data;
}

py::dict diagnostic_to_dict(const circuitsim::DiagnosticMessage& diagnostic) {
    py::dict data;
    data["severity"] =
        diagnostic.severity == circuitsim::DiagnosticSeverity::error ? "error" : "warning";
    data["code"] = diagnostic.code;
    data["message"] = diagnostic.message;
    return data;
}

py::list diagnostics_to_list(const std::vector<circuitsim::DiagnosticMessage>& diagnostics) {
    py::list values;
    for (const auto& diagnostic : diagnostics) {
        values.append(diagnostic_to_dict(diagnostic));
    }
    return values;
}

}  // namespace

PYBIND11_MODULE(circuitsim_py, module) {
    module.doc() = "Python bindings for CircuitSim";

    module.def("parse_netlist", [](const std::string& text) {
        const auto circuit = parse_or_throw(text);
        py::dict data;
        py::list components;
        for (const auto& component : circuit.components) {
            components.append(component_to_dict(component));
        }
        data["nodes"] = circuit.nodes;
        data["components"] = components;
        return data;
    });

    module.def("diagnose_netlist", [](const std::string& text) {
        const auto circuit = parse_or_throw(text);
        const auto diagnostics = circuitsim::analyze_circuit(circuit);
        py::dict data;
        data["messages"] = diagnostics_to_list(diagnostics.messages);
        data["has_errors"] = diagnostics.has_errors();
        return data;
    });

    module.def("run_dc", [](const std::string& text) {
        const auto circuit = parse_or_throw(text);
        const circuitsim::DCSolver solver;
        const auto result = solver.solve(circuit);
        if (!result.ok()) {
            std::string message = "DC solve failed:";
            for (const auto& error : result.errors) {
                message += "\n" + error.message;
            }
            throw std::runtime_error(message);
        }

        py::dict data;
        data["diagnostics"] = diagnostics_to_list(result.diagnostics);
        data["node_voltages"] = result.node_voltages;
        data["source_currents"] = result.source_currents;
        return data;
    });

    module.def("run_ac", [](const std::string& text, const std::vector<double>& frequencies_hz) {
        const auto circuit = parse_or_throw(text);
        const circuitsim::ACSolver solver;
        const auto result = solver.solve(circuit, frequencies_hz);
        if (!result.ok()) {
            std::string message = "AC solve failed:";
            for (const auto& error : result.errors) {
                message += "\n" + error.message;
            }
            throw std::runtime_error(message);
        }

        py::dict node_voltages;
        for (const auto& [node, values] : result.node_voltages) {
            py::list samples;
            for (const auto& value : values) {
                py::dict sample;
                sample["real"] = value.real();
                sample["imag"] = value.imag();
                sample["magnitude"] = std::abs(value);
                sample["phase_rad"] = std::arg(value);
                samples.append(sample);
            }
            node_voltages[node.c_str()] = samples;
        }

        py::dict source_currents;
        for (const auto& [source, values] : result.source_currents) {
            py::list samples;
            for (const auto& value : values) {
                py::dict sample;
                sample["real"] = value.real();
                sample["imag"] = value.imag();
                sample["magnitude"] = std::abs(value);
                sample["phase_rad"] = std::arg(value);
                samples.append(sample);
            }
            source_currents[source.c_str()] = samples;
        }

        py::dict data;
        data["diagnostics"] = diagnostics_to_list(result.diagnostics);
        data["frequencies_hz"] = result.frequencies_hz;
        data["node_voltages"] = node_voltages;
        data["source_currents"] = source_currents;
        return data;
    });

    module.def("run_transient", [](const std::string& text, double time_step, double stop_time) {
        const auto circuit = parse_or_throw(text);
        const circuitsim::TransientSolver solver;
        const auto result = solver.solve(
            circuit,
            circuitsim::TransientAnalysisConfig{
                .time_step = time_step,
                .stop_time = stop_time,
            }
        );

        if (!result.ok()) {
            std::string message = "Transient solve failed:";
            for (const auto& error : result.errors) {
                message += "\n" + error.message;
            }
            throw std::runtime_error(message);
        }

        py::dict data;
        data["diagnostics"] = diagnostics_to_list(result.diagnostics);
        data["time_points"] = result.time_points;
        data["node_voltages"] = result.node_voltages;
        data["source_currents"] = result.source_currents;
        return data;
    });
}
