#pragma once

#include <cstddef>
#include <string>

namespace circuitsim {

struct AnalysisSummary {
    std::string analysis_type;
    std::size_t node_count = 0;
    std::size_t component_count = 0;
    std::size_t resistor_count = 0;
    std::size_t capacitor_count = 0;
    std::size_t inductor_count = 0;
    std::size_t voltage_source_count = 0;
    std::size_t current_source_count = 0;
    std::size_t sample_count = 0;
    double time_step = 0.0;
    double stop_time = 0.0;
};

}  // namespace circuitsim
