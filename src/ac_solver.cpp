#include "circuitsim/ac_solver.h"

#include <complex>
#include <numbers>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "circuitsim/diagnostics.h"
#include "solver_utils.h"

namespace circuitsim {
namespace {

void add_error(ACSolveResult& result, std::string message) {
    result.errors.push_back(SolveError{std::move(message)});
}

}  // namespace

ACSolveResult ACSolver::solve(
    const Circuit& circuit,
    const std::vector<double>& frequencies_hz
) const {
    ACSolveResult result;

    const CircuitDiagnostics diagnostics = analyze_circuit(circuit);
    result.diagnostics = diagnostics.messages;
    if (diagnostics.has_errors()) {
        for (const auto& diagnostic : diagnostics.messages) {
            if (diagnostic.severity == DiagnosticSeverity::error) {
                add_error(result, diagnostic.message);
            }
        }
        return result;
    }

    if (frequencies_hz.empty()) {
        add_error(result, "at least one AC frequency is required");
        return result;
    }
    for (const double frequency : frequencies_hz) {
        if (!(frequency > 0.0)) {
            add_error(result, "AC frequencies must be positive");
            return result;
        }
    }

    std::unordered_map<std::string, int> node_index;
    int next_node_index = 0;
    for (const auto& node : circuit.nodes) {
        if (!is_ground_node(node)) {
            node_index.emplace(node, next_node_index++);
            result.node_voltages.emplace(node, std::vector<std::complex<double>>{});
            result.node_voltages[node].reserve(frequencies_hz.size());
        }
    }
    result.node_voltages.emplace("0", std::vector<std::complex<double>>(frequencies_hz.size(), {0.0, 0.0}));

    std::vector<const Component*> voltage_sources;
    for (const auto& component : circuit.components) {
        if (component.type == ComponentType::voltage_source) {
            voltage_sources.push_back(&component);
            result.source_currents.emplace(component.id, std::vector<std::complex<double>>{});
            result.source_currents[component.id].reserve(frequencies_hz.size());
        }
    }

    const std::size_t node_count = static_cast<std::size_t>(next_node_index);
    const std::size_t voltage_source_count = voltage_sources.size();
    const std::size_t system_size = node_count + voltage_source_count;
    if (system_size == 0) {
        add_error(result, "circuit does not contain any solvable unknowns");
        return result;
    }

    std::unordered_map<std::string, std::size_t> source_index;
    for (std::size_t index = 0; index < voltage_source_count; ++index) {
        source_index.emplace(voltage_sources[index]->id, node_count + index);
    }

    const auto get_node_index = [&node_index](const std::string& node) -> int {
        if (is_ground_node(node)) {
            return -1;
        }
        return node_index.at(node);
    };

    for (const double frequency_hz : frequencies_hz) {
        result.frequencies_hz.push_back(frequency_hz);
        const double omega = 2.0 * std::numbers::pi * frequency_hz;

        std::vector<std::vector<std::complex<double>>> matrix(
            system_size,
            std::vector<std::complex<double>>(system_size, {0.0, 0.0})
        );
        std::vector<std::complex<double>> rhs(system_size, {0.0, 0.0});

        for (const auto& component : circuit.components) {
            const int positive = get_node_index(component.node_positive);
            const int negative = get_node_index(component.node_negative);

            switch (component.type) {
                case ComponentType::resistor: {
                    const std::complex<double> conductance{1.0 / component.value, 0.0};
                    if (positive >= 0) {
                        matrix[positive][positive] += conductance;
                    }
                    if (negative >= 0) {
                        matrix[negative][negative] += conductance;
                    }
                    if (positive >= 0 && negative >= 0) {
                        matrix[positive][negative] -= conductance;
                        matrix[negative][positive] -= conductance;
                    }
                    break;
                }
                case ComponentType::capacitor: {
                    const std::complex<double> admittance{0.0, omega * component.value};
                    if (positive >= 0) {
                        matrix[positive][positive] += admittance;
                    }
                    if (negative >= 0) {
                        matrix[negative][negative] += admittance;
                    }
                    if (positive >= 0 && negative >= 0) {
                        matrix[positive][negative] -= admittance;
                        matrix[negative][positive] -= admittance;
                    }
                    break;
                }
                case ComponentType::inductor: {
                    const std::complex<double> admittance{0.0, -1.0 / (omega * component.value)};
                    if (positive >= 0) {
                        matrix[positive][positive] += admittance;
                    }
                    if (negative >= 0) {
                        matrix[negative][negative] += admittance;
                    }
                    if (positive >= 0 && negative >= 0) {
                        matrix[positive][negative] -= admittance;
                        matrix[negative][positive] -= admittance;
                    }
                    break;
                }
                case ComponentType::current_source: {
                    const std::complex<double> current{component.value, 0.0};
                    if (positive >= 0) {
                        rhs[positive] -= current;
                    }
                    if (negative >= 0) {
                        rhs[negative] += current;
                    }
                    break;
                }
                case ComponentType::voltage_source: {
                    const std::size_t index = source_index.at(component.id);
                    if (positive >= 0) {
                        matrix[positive][index] += 1.0;
                        matrix[index][positive] += 1.0;
                    }
                    if (negative >= 0) {
                        matrix[negative][index] -= 1.0;
                        matrix[index][negative] -= 1.0;
                    }
                    rhs[index] += std::complex<double>{component.value, 0.0};
                    break;
                }
            }
        }

        try {
            const auto solution = solve_complex_linear_system(matrix, rhs);
            for (const auto& [node, index] : node_index) {
                result.node_voltages[node].push_back(solution[static_cast<std::size_t>(index)]);
            }
            for (const auto* source : voltage_sources) {
                result.source_currents[source->id].push_back(solution[source_index.at(source->id)]);
            }
        } catch (const std::runtime_error& error) {
            add_error(result, error.what());
            return result;
        }
    }

    return result;
}

}  // namespace circuitsim
