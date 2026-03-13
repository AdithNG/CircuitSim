#include "circuitsim/diagnostics.h"

#include <algorithm>
#include <queue>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "solver_utils.h"

namespace circuitsim {
namespace {

void add_message(
    CircuitDiagnostics& diagnostics,
    DiagnosticSeverity severity,
    std::string code,
    std::string message
) {
    diagnostics.messages.push_back(DiagnosticMessage{
        .severity = severity,
        .code = std::move(code),
        .message = std::move(message),
    });
}

std::string join_names(const std::vector<std::string>& values) {
    std::string joined;
    for (std::size_t index = 0; index < values.size(); ++index) {
        if (index > 0) {
            joined += ", ";
        }
        joined += values[index];
    }
    return joined;
}

}  // namespace

bool CircuitDiagnostics::has_errors() const {
    return std::any_of(messages.begin(), messages.end(), [](const DiagnosticMessage& message) {
        return message.severity == DiagnosticSeverity::error;
    });
}

CircuitDiagnostics analyze_circuit(const Circuit& circuit) {
    CircuitDiagnostics diagnostics;

    if (circuit.components.empty()) {
        add_message(
            diagnostics,
            DiagnosticSeverity::error,
            "empty-circuit",
            "Circuit does not contain any components."
        );
        return diagnostics;
    }

    std::unordered_map<std::string, std::vector<std::string>> adjacency;
    adjacency.reserve(circuit.nodes.size());

    bool has_ground = false;
    for (const auto& node : circuit.nodes) {
        adjacency[node];
        if (is_ground_node(node)) {
            has_ground = true;
        }
    }

    if (!has_ground) {
        add_message(
            diagnostics,
            DiagnosticSeverity::error,
            "missing-ground",
            "Circuit must contain a ground node named 0 or GND."
        );
        return diagnostics;
    }

    for (const auto& component : circuit.components) {
        adjacency[component.node_positive].push_back(component.node_negative);
        adjacency[component.node_negative].push_back(component.node_positive);
    }

    std::queue<std::string> frontier;
    std::unordered_set<std::string> visited;

    for (const auto& node : circuit.nodes) {
        if (is_ground_node(node)) {
            frontier.push(node);
            visited.insert(node);
        }
    }

    while (!frontier.empty()) {
        const std::string node = frontier.front();
        frontier.pop();

        for (const auto& neighbor : adjacency[node]) {
            if (visited.insert(neighbor).second) {
                frontier.push(neighbor);
            }
        }
    }

    std::vector<std::string> floating_nodes;
    for (const auto& node : circuit.nodes) {
        if (!is_ground_node(node) && !visited.contains(node)) {
            floating_nodes.push_back(node);
        }
    }
    std::sort(floating_nodes.begin(), floating_nodes.end());

    if (!floating_nodes.empty()) {
        add_message(
            diagnostics,
            DiagnosticSeverity::error,
            "floating-nodes",
            "Nodes are not connected to ground: " + join_names(floating_nodes) + "."
        );

        std::vector<std::string> floating_components;
        for (const auto& component : circuit.components) {
            if (std::find(floating_nodes.begin(), floating_nodes.end(), component.node_positive) != floating_nodes.end() ||
                std::find(floating_nodes.begin(), floating_nodes.end(), component.node_negative) != floating_nodes.end()) {
                floating_components.push_back(component.id);
            }
        }
        std::sort(floating_components.begin(), floating_components.end());
        floating_components.erase(
            std::unique(floating_components.begin(), floating_components.end()),
            floating_components.end()
        );

        if (!floating_components.empty()) {
            add_message(
                diagnostics,
                DiagnosticSeverity::warning,
                "floating-components",
                "Components in floating regions: " + join_names(floating_components) + "."
            );
        }
    }

    bool has_dc_path = false;
    for (const auto& component : circuit.components) {
        if (component.type == ComponentType::resistor ||
            component.type == ComponentType::inductor ||
            component.type == ComponentType::current_source ||
            component.type == ComponentType::voltage_source) {
            has_dc_path = true;
            break;
        }
    }

    if (!has_dc_path) {
        add_message(
            diagnostics,
            DiagnosticSeverity::warning,
            "reactive-only-network",
            "Circuit contains no DC-conductive elements; DC analysis will likely fail."
        );
    }

    return diagnostics;
}

}  // namespace circuitsim
