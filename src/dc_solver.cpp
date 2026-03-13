#include "circuitsim/dc_solver.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace circuitsim {
namespace {

constexpr double kPivotTolerance = 1e-12;

bool is_ground_node(const std::string& node_name) {
    if (node_name == "0") {
        return true;
    }

    std::string normalized;
    normalized.reserve(node_name.size());
    for (const char character : node_name) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }

    return normalized == "gnd";
}

void add_error(DCSolveResult& result, std::string message) {
    result.errors.push_back(SolveError{std::move(message)});
}

std::vector<double> solve_linear_system(std::vector<std::vector<double>> matrix, std::vector<double> rhs) {
    const std::size_t size = matrix.size();
    for (std::size_t pivot = 0; pivot < size; ++pivot) {
        std::size_t best_row = pivot;
        double best_value = std::fabs(matrix[pivot][pivot]);

        for (std::size_t candidate = pivot + 1; candidate < size; ++candidate) {
            const double candidate_value = std::fabs(matrix[candidate][pivot]);
            if (candidate_value > best_value) {
                best_value = candidate_value;
                best_row = candidate;
            }
        }

        if (best_value < kPivotTolerance) {
            throw std::runtime_error("matrix is singular");
        }

        if (best_row != pivot) {
            std::swap(matrix[pivot], matrix[best_row]);
            std::swap(rhs[pivot], rhs[best_row]);
        }

        const double pivot_value = matrix[pivot][pivot];
        for (std::size_t row = pivot + 1; row < size; ++row) {
            const double factor = matrix[row][pivot] / pivot_value;
            if (std::fabs(factor) < kPivotTolerance) {
                continue;
            }

            for (std::size_t column = pivot; column < size; ++column) {
                matrix[row][column] -= factor * matrix[pivot][column];
            }
            rhs[row] -= factor * rhs[pivot];
        }
    }

    std::vector<double> solution(size, 0.0);
    for (std::size_t row = size; row-- > 0;) {
        double value = rhs[row];
        for (std::size_t column = row + 1; column < size; ++column) {
            value -= matrix[row][column] * solution[column];
        }
        solution[row] = value / matrix[row][row];
    }

    return solution;
}

}  // namespace

DCSolveResult DCSolver::solve(const Circuit& circuit) const {
    DCSolveResult result;

    if (circuit.components.empty()) {
        add_error(result, "circuit has no components");
        return result;
    }

    std::unordered_map<std::string, int> node_index;
    node_index.reserve(circuit.nodes.size());

    int next_node_index = 0;
    bool found_ground = false;
    for (const auto& node : circuit.nodes) {
        if (is_ground_node(node)) {
            found_ground = true;
        } else {
            node_index.emplace(node, next_node_index++);
        }
    }

    if (!found_ground) {
        add_error(result, "circuit must contain a ground node named 0 or GND");
        return result;
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
