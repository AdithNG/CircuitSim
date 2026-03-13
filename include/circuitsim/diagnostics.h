#pragma once

#include <string>
#include <vector>

#include "circuitsim/circuit.h"

namespace circuitsim {

enum class DiagnosticSeverity {
    warning,
    error,
};

struct DiagnosticMessage {
    DiagnosticSeverity severity = DiagnosticSeverity::warning;
    std::string code;
    std::string message;
};

struct CircuitDiagnostics {
    std::vector<DiagnosticMessage> messages;

    [[nodiscard]] bool has_errors() const;
};

[[nodiscard]] CircuitDiagnostics analyze_circuit(const Circuit& circuit);

}  // namespace circuitsim
