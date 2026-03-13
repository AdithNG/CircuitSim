#pragma once

#include "circuitsim/analysis_summary.h"
#include "circuitsim/circuit.h"

namespace circuitsim {

inline AnalysisSummary make_base_summary(const Circuit& circuit, const char* analysis_type) {
    AnalysisSummary summary;
    summary.analysis_type = analysis_type;
    summary.node_count = circuit.nodes.size();
    summary.component_count = circuit.components.size();

    for (const auto& component : circuit.components) {
        switch (component.type) {
            case ComponentType::resistor:
                ++summary.resistor_count;
                break;
            case ComponentType::capacitor:
                ++summary.capacitor_count;
                break;
            case ComponentType::inductor:
                ++summary.inductor_count;
                break;
            case ComponentType::voltage_source:
                ++summary.voltage_source_count;
                break;
            case ComponentType::current_source:
                ++summary.current_source_count;
                break;
        }
    }

    return summary;
}

}  // namespace circuitsim
