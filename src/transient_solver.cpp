#include "circuitsim/transient_solver.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "solver_utils.h"

namespace circuitsim {
namespace {

void add_error(TransientSolveResult& result, std::string message) {
    result.errors.push_back(SolveError{std::move(message)});
}

}  // namespace

TransientSolveResult TransientSolver::solve(
    const Circuit& circuit,
    const TransientAnalysisConfig& config
) const {
    TransientSolveResult result;

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

    if (circuit.components.empty()) {
        add_error(result, "circuit has no components");
        return result;
    }
    if (!(config.time_step > 0.0)) {
        add_error(result, "time_step must be positive");
        return result;
    }
    if (!(config.stop_time > 0.0)) {
        add_error(result, "stop_time must be positive");
        return result;
    }

    std::unordered_map<std::string, int> node_index;
    node_index.reserve(circuit.nodes.size());

    int next_node_index = 0;
    for (const auto& node : circuit.nodes) {
        if (!is_ground_node(node)) {
            node_index.emplace(node, next_node_index++);
        }
    }

    std::vector<const Component*> voltage_sources;
    std::vector<const Component*> inductors;
    for (const auto& component : circuit.components) {
        if (component.type == ComponentType::voltage_source) {
            voltage_sources.push_back(&component);
        }
        if (component.type == ComponentType::inductor) {
            inductors.push_back(&component);
        }
    }

    const std::size_t node_count = static_cast<std::size_t>(next_node_index);
    const std::size_t voltage_source_count = voltage_sources.size();
    const std::size_t inductor_count = inductors.size();
    const std::size_t system_size = node_count + voltage_source_count + inductor_count;

    if (system_size == 0) {
        add_error(result, "circuit does not contain any solvable unknowns");
        return result;
    }

    std::unordered_map<std::string, std::size_t> source_index;
    for (std::size_t index = 0; index < voltage_source_count; ++index) {
        source_index.emplace(voltage_sources[index]->id, node_count + index);
    }
    std::unordered_map<std::string, std::size_t> inductor_index;
    for (std::size_t index = 0; index < inductor_count; ++index) {
        inductor_index.emplace(inductors[index]->id, node_count + voltage_source_count + index);
    }

    const auto get_node_index = [&node_index](const std::string& node) -> int {
        if (is_ground_node(node)) {
            return -1;
        }
        return node_index.at(node);
    };

    const std::size_t step_count = static_cast<std::size_t>(std::ceil(config.stop_time / config.time_step));
    std::vector<double> previous_solution(system_size, 0.0);

    result.node_voltages.emplace("0", std::vector<double>(step_count, 0.0));
    for (const auto& [node, index] : node_index) {
        (void)index;
        result.node_voltages.emplace(node, std::vector<double>{});
        result.node_voltages[node].reserve(step_count);
    }
    for (const auto* source : voltage_sources) {
        result.source_currents.emplace(source->id, std::vector<double>{});
        result.source_currents[source->id].reserve(step_count);
    }

    for (std::size_t step = 0; step < step_count; ++step) {
        std::vector<std::vector<double>> matrix(system_size, std::vector<double>(system_size, 0.0));
        std::vector<double> rhs(system_size, 0.0);

        for (const auto& component : circuit.components) {
            const int positive = get_node_index(component.node_positive);
            const int negative = get_node_index(component.node_negative);

            switch (component.type) {
                case ComponentType::resistor: {
                    const double conductance = 1.0 / component.value;
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
                case ComponentType::current_source: {
                    if (positive >= 0) {
                        rhs[positive] -= component.value;
                    }
                    if (negative >= 0) {
                        rhs[negative] += component.value;
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
                    rhs[index] += component.value;
                    break;
                }
                case ComponentType::inductor: {
                    const std::size_t index = inductor_index.at(component.id);
                    const double history_factor = component.value / config.time_step;
                    if (positive >= 0) {
                        matrix[positive][index] += 1.0;
                        matrix[index][positive] += 1.0;
                    }
                    if (negative >= 0) {
                        matrix[negative][index] -= 1.0;
                        matrix[index][negative] -= 1.0;
                    }
                    matrix[index][index] -= history_factor;
                    rhs[index] -= history_factor * previous_solution[index];
                    break;
                }
                case ComponentType::capacitor: {
                    const double conductance = component.value / config.time_step;
                    double previous_voltage = 0.0;
                    if (positive >= 0) {
                        previous_voltage += previous_solution[static_cast<std::size_t>(positive)];
                        matrix[positive][positive] += conductance;
                    }
                    if (negative >= 0) {
                        previous_voltage -= previous_solution[static_cast<std::size_t>(negative)];
                        matrix[negative][negative] += conductance;
                    }
                    if (positive >= 0 && negative >= 0) {
                        matrix[positive][negative] -= conductance;
                        matrix[negative][positive] -= conductance;
                    }
                    const double history_current = conductance * previous_voltage;
                    if (positive >= 0) {
                        rhs[positive] += history_current;
                    }
                    if (negative >= 0) {
                        rhs[negative] -= history_current;
                    }
                    break;
                }
            }
        }

        try {
            const std::vector<double> solution = solve_linear_system(matrix, rhs);
            previous_solution = solution;

            result.time_points.push_back((static_cast<double>(step) + 1.0) * config.time_step);
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
