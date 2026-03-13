#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "circuitsim/circuit.h"

namespace circuitsim {

struct SolveError {
    std::string message;
};

struct DCSolveResult {
    std::unordered_map<std::string, double> node_voltages;
    std::unordered_map<std::string, double> source_currents;
    std::vector<SolveError> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

class DCSolver {
public:
    [[nodiscard]] DCSolveResult solve(const Circuit& circuit) const;
};

}  // namespace circuitsim
