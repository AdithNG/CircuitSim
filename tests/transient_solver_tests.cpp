#include "circuitsim/netlist_parser.h"
#include "circuitsim/transient_solver.h"
#include "test_framework.h"

#include <string>

using circuitsim::NetlistParser;
using circuitsim::TransientAnalysisConfig;
using circuitsim::TransientSolver;

namespace {

circuitsim::Circuit parse_transient_circuit(const std::string& netlist) {
    const NetlistParser parser;
    const auto parse_result = parser.parse_text(netlist);
    EXPECT_TRUE(parse_result.ok());
    return parse_result.circuit;
}

}  // namespace

TEST_CASE(transient_solver_simulates_rc_charge) {
    const auto circuit = parse_transient_circuit(
        "V1 in 0 5\n"
        "R1 in out 1k\n"
        "C1 out 0 1u\n");

    const TransientSolver solver;
    const auto result = solver.solve(circuit, TransientAnalysisConfig{.time_step = 1e-4, .stop_time = 5e-3});

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.time_points.size(), 50U);
    EXPECT_EQ(result.node_voltages.at("out").size(), 50U);
    EXPECT_NEAR(result.time_points.front(), 1e-4, 1e-12);
    EXPECT_NEAR(result.time_points.back(), 5e-3, 1e-12);
    EXPECT_TRUE(result.node_voltages.at("out").front() > 0.0);
    EXPECT_TRUE(result.node_voltages.at("out").back() > 4.9);
    EXPECT_TRUE(result.node_voltages.at("out").back() < 5.01);
}

TEST_CASE(transient_solver_rc_response_is_monotonic_for_charge_case) {
    const auto circuit = parse_transient_circuit(
        "V1 in 0 3.3\n"
        "R1 in out 2k\n"
        "C1 out 0 2u\n");

    const TransientSolver solver;
    const auto result = solver.solve(circuit, TransientAnalysisConfig{.time_step = 2e-4, .stop_time = 4e-3});

    EXPECT_TRUE(result.ok());
    const auto& samples = result.node_voltages.at("out");
    for (std::size_t index = 1; index < samples.size(); ++index) {
        EXPECT_TRUE(samples[index] >= samples[index - 1]);
    }
}

TEST_CASE(transient_solver_supports_current_source_into_rc) {
    const auto circuit = parse_transient_circuit(
        "I1 in 0 -1m\n"
        "R1 in 0 10k\n"
        "C1 in 0 1u\n");

    const TransientSolver solver;
    const auto result = solver.solve(circuit, TransientAnalysisConfig{.time_step = 1e-3, .stop_time = 5e-2});

    EXPECT_TRUE(result.ok());
    EXPECT_TRUE(result.node_voltages.at("in").back() > 9.0);
    EXPECT_TRUE(result.node_voltages.at("in").back() < 10.1);
}

TEST_CASE(transient_solver_rejects_invalid_time_config) {
    const auto circuit = parse_transient_circuit(
        "V1 in 0 5\n"
        "R1 in out 1k\n"
        "C1 out 0 1u\n");

    const TransientSolver solver;
    const auto zero_step = solver.solve(circuit, TransientAnalysisConfig{.time_step = 0.0, .stop_time = 1e-3});
    const auto zero_stop = solver.solve(circuit, TransientAnalysisConfig{.time_step = 1e-4, .stop_time = 0.0});

    EXPECT_FALSE(zero_step.ok());
    EXPECT_FALSE(zero_stop.ok());
    EXPECT_EQ(zero_step.errors.size(), 1U);
    EXPECT_EQ(zero_stop.errors.size(), 1U);
}

TEST_CASE(transient_solver_rejects_inductors_for_now) {
    const auto circuit = parse_transient_circuit(
        "V1 in 0 5\n"
        "L1 in 0 1u\n");

    const TransientSolver solver;
    const auto result = solver.solve(circuit, TransientAnalysisConfig{.time_step = 1e-4, .stop_time = 1e-3});

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(transient_solver_rejects_missing_ground) {
    const auto circuit = parse_transient_circuit(
        "V1 in ref 5\n"
        "R1 in out 1k\n"
        "C1 out ref 1u\n");

    const TransientSolver solver;
    const auto result = solver.solve(circuit, TransientAnalysisConfig{.time_step = 1e-4, .stop_time = 1e-3});

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}
