#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "circuitsim/analysis_summary.h"
#include "circuitsim/circuit.h"
#include "circuitsim/dc_solver.h"

namespace circuitsim {

struct TransientAnalysisConfig {
    double time_step = 0.0;
    double stop_time = 0.0;
};

struct TransientSolveResult {
    AnalysisSummary summary;
    std::vector<double> time_points;
    std::unordered_map<std::string, std::vector<double>> node_voltages;
    std::unordered_map<std::string, std::vector<double>> source_currents;
    std::vector<DiagnosticMessage> diagnostics;
    std::vector<SolveError> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

class TransientSolver {
public:
    [[nodiscard]] TransientSolveResult solve(
        const Circuit& circuit,
        const TransientAnalysisConfig& config
    ) const;
};

}  // namespace circuitsim
