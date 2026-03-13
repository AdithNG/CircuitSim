#include "circuitsim/ac_solver.h"
#include "circuitsim/netlist_parser.h"
#include "test_framework.h"

#include <cmath>
#include <numbers>
#include <string>

using circuitsim::ACSolver;
using circuitsim::NetlistParser;

namespace {

circuitsim::Circuit parse_ac_circuit(const std::string& netlist) {
    const NetlistParser parser;
    const auto result = parser.parse_text(netlist);
    EXPECT_TRUE(result.ok());
    return result.circuit;
}

double magnitude(const std::complex<double>& value) {
    return std::abs(value);
}

double phase(const std::complex<double>& value) {
    return std::arg(value);
}

}  // namespace

TEST_CASE(ac_solver_matches_rc_lowpass_cutoff_response) {
    const auto circuit = parse_ac_circuit(
        "V1 in 0 1\n"
        "R1 in out 1k\n"
        "C1 out 0 1n\n");

    const ACSolver solver;
    const auto result = solver.solve(circuit, {159154.94309189535});

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.frequencies_hz.size(), 1U);
    EXPECT_NEAR(magnitude(result.node_voltages.at("in")[0]), 1.0, 1e-9);
    EXPECT_NEAR(magnitude(result.node_voltages.at("out")[0]), std::sqrt(0.5), 1e-3);
    EXPECT_NEAR(phase(result.node_voltages.at("out")[0]), -std::numbers::pi / 4.0, 1e-3);
}

TEST_CASE(ac_solver_shows_lowpass_rolloff) {
    const auto circuit = parse_ac_circuit(
        "V1 in 0 1\n"
        "R1 in out 1k\n"
        "C1 out 0 1n\n");

    const ACSolver solver;
    const auto result = solver.solve(circuit, {1e3, 1e6});

    EXPECT_TRUE(result.ok());
    const auto& out = result.node_voltages.at("out");
    EXPECT_TRUE(magnitude(out[0]) > magnitude(out[1]));
    EXPECT_TRUE(magnitude(out[0]) > 0.99);
    EXPECT_TRUE(magnitude(out[1]) < 0.2);
}

TEST_CASE(ac_solver_rejects_invalid_frequencies) {
    const auto circuit = parse_ac_circuit(
        "V1 in 0 1\n"
        "R1 in out 1k\n"
        "C1 out 0 1n\n");

    const ACSolver solver;
    const auto result = solver.solve(circuit, {0.0});

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(ac_solver_matches_rl_lowpass_cutoff_response) {
    const auto circuit = parse_ac_circuit(
        "V1 in 0 1\n"
        "L1 in out 1m\n"
        "R1 out 0 100\n");

    const ACSolver solver;
    const auto result = solver.solve(circuit, {15915.494309189535});

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.frequencies_hz.size(), 1U);
    EXPECT_NEAR(magnitude(result.node_voltages.at("out")[0]), std::sqrt(0.5), 1e-3);
    EXPECT_NEAR(phase(result.node_voltages.at("out")[0]), -std::numbers::pi / 4.0, 1e-3);
}

TEST_CASE(ac_solver_shows_rl_lowpass_rolloff) {
    const auto circuit = parse_ac_circuit(
        "V1 in 0 1\n"
        "L1 in out 1m\n"
        "R1 out 0 100\n");

    const ACSolver solver;
    const auto result = solver.solve(circuit, {1e3, 1e6});

    EXPECT_TRUE(result.ok());
    const auto& out = result.node_voltages.at("out");
    EXPECT_TRUE(magnitude(out[0]) > magnitude(out[1]));
    EXPECT_TRUE(magnitude(out[0]) > 0.99);
    EXPECT_TRUE(magnitude(out[1]) < 0.05);
}
