#include "circuitsim/dc_solver.h"

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "solver_utils.h"

namespace circuitsim {
namespace {

void add_error(DCSolveResult& result, std::string message) {
    result.errors.push_back(SolveError{std::move(message)});
}
}  // namespace

DCSolveResult DCSolver::solve(const Circuit& circuit) const {
    DCSolveResult result;

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

    std::unordered_map<std::string, int> node_index;
    node_index.reserve(circuit.nodes.size());

    int next_node_index = 0;
    for (const auto& node : circuit.nodes) {
        if (!is_ground_node(node)) {
            node_index.emplace(node, next_node_index++);
        }
    }

    std::vector<const Component*> voltage_sources;
    for (const auto& component : circuit.components) {
        if (component.type == ComponentType::capacitor || component.type == ComponentType::inductor) {
            add_error(result, "DC solver does not yet support capacitors or inductors");
            return result;
        }
        if (component.type == ComponentType::voltage_source) {
            voltage_sources.push_back(&component);
        }
    }

    const std::size_t node_count = static_cast<std::size_t>(next_node_index);
    const std::size_t voltage_source_count = voltage_sources.size();
    const std::size_t system_size = node_count + voltage_source_count;

    if (system_size == 0) {
        add_error(result, "circuit does not contain any solvable unknowns");
        return result;
    }

    std::vector<std::vector<double>> matrix(system_size, std::vector<double>(system_size, 0.0));
    std::vector<double> rhs(system_size, 0.0);

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
            case ComponentType::capacitor:
            case ComponentType::inductor:
                break;
        }
    }

    try {
        const std::vector<double> solution = solve_linear_system(matrix, rhs);

        result.node_voltages.emplace("0", 0.0);
        for (const auto& [node, index] : node_index) {
            result.node_voltages.emplace(node, solution[static_cast<std::size_t>(index)]);
        }
        for (const auto& component : voltage_sources) {
            const std::size_t index = source_index.at(component->id);
            result.source_currents.emplace(component->id, solution[index]);
        }
    } catch (const std::runtime_error& error) {
        add_error(result, error.what());
    }

    return result;
}

}  // namespace circuitsim
