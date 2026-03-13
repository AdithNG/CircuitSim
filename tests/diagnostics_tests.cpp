#include "circuitsim/diagnostics.h"
#include "circuitsim/netlist_parser.h"
#include "test_framework.h"

#include <string>

using circuitsim::DiagnosticSeverity;
using circuitsim::NetlistParser;
using circuitsim::analyze_circuit;

namespace {

circuitsim::Circuit parse_diagnostics_circuit(const std::string& netlist) {
    const NetlistParser parser;
    const auto result = parser.parse_text(netlist);
    EXPECT_TRUE(result.ok());
    return result.circuit;
}

}  // namespace

TEST_CASE(diagnostics_reports_missing_ground) {
    const auto circuit = parse_diagnostics_circuit(
        "V1 in ref 5\n"
        "R1 in ref 1k\n");

    const auto diagnostics = analyze_circuit(circuit);

    EXPECT_TRUE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics.messages.size(), 1U);
    EXPECT_EQ(diagnostics.messages[0].code, "missing-ground");
}

TEST_CASE(diagnostics_reports_floating_nodes_and_components) {
    const auto circuit = parse_diagnostics_circuit(
        "V1 in 0 5\n"
        "R1 in out 1k\n"
        "R2 out 0 2k\n"
        "R3 a b 10\n");

    const auto diagnostics = analyze_circuit(circuit);

    EXPECT_TRUE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics.messages.size(), 2U);
    EXPECT_EQ(diagnostics.messages[0].code, "floating-nodes");
    EXPECT_EQ(diagnostics.messages[1].code, "floating-components");
}

TEST_CASE(diagnostics_warns_for_reactive_only_networks) {
    const auto circuit = parse_diagnostics_circuit(
        "C1 in 0 1u\n"
        "C2 in out 1u\n");

    const auto diagnostics = analyze_circuit(circuit);

    EXPECT_FALSE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics.messages.size(), 1U);
    EXPECT_EQ(diagnostics.messages[0].severity, DiagnosticSeverity::warning);
    EXPECT_EQ(diagnostics.messages[0].code, "reactive-only-network");
}

TEST_CASE(diagnostics_is_quiet_for_grounded_linear_circuit) {
    const auto circuit = parse_diagnostics_circuit(
        "V1 in 0 5\n"
        "R1 in out 1k\n"
        "R2 out 0 2k\n");

    const auto diagnostics = analyze_circuit(circuit);

    EXPECT_FALSE(diagnostics.has_errors());
    EXPECT_EQ(diagnostics.messages.size(), 0U);
}
