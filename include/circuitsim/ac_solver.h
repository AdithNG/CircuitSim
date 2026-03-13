#pragma once

#include <complex>
#include <string>
#include <unordered_map>
#include <vector>

#include "circuitsim/circuit.h"
#include "circuitsim/dc_solver.h"

namespace circuitsim {

struct ACSolveResult {
    std::vector<double> frequencies_hz;
    std::unordered_map<std::string, std::vector<std::complex<double>>> node_voltages;
    std::unordered_map<std::string, std::vector<std::complex<double>>> source_currents;
    std::vector<DiagnosticMessage> diagnostics;
    std::vector<SolveError> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

class ACSolver {
public:
    [[nodiscard]] ACSolveResult solve(
        const Circuit& circuit,
        const std::vector<double>& frequencies_hz
    ) const;
};

}  // namespace circuitsim
