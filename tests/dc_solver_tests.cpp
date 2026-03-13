#include "circuitsim/dc_solver.h"
#include "circuitsim/netlist_parser.h"
#include "test_framework.h"

#include <string>

using circuitsim::DCSolver;
using circuitsim::NetlistParser;

namespace {

circuitsim::Circuit parse_or_fail(const std::string& netlist) {
    const NetlistParser parser;
    const auto parse_result = parser.parse_text(netlist);
    EXPECT_TRUE(parse_result.ok());
    return parse_result.circuit;
}

}  // namespace

TEST_CASE(dc_solver_solves_resistor_divider) {
    const auto circuit = parse_or_fail(
        "V1 in 0 12\n"
        "R1 in out 1k\n"
        "R2 out 0 1k\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_TRUE(result.ok());
    EXPECT_NEAR(result.node_voltages.at("in"), 12.0, 1e-9);
    EXPECT_NEAR(result.node_voltages.at("out"), 6.0, 1e-9);
    EXPECT_NEAR(result.source_currents.at("V1"), -0.006, 1e-12);
}

TEST_CASE(dc_solver_solves_current_source_network) {
    const auto circuit = parse_or_fail(
        "I1 in 0 2m\n"
        "R1 in 0 500\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_TRUE(result.ok());
    EXPECT_NEAR(result.node_voltages.at("in"), -1.0, 1e-9);
}

TEST_CASE(dc_solver_handles_multiple_voltage_sources) {
    const auto circuit = parse_or_fail(
        "V1 in 0 10\n"
        "V2 aux 0 3\n"
        "R1 in aux 1k\n"
        "R2 aux 0 1k\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_TRUE(result.ok());
    EXPECT_NEAR(result.node_voltages.at("in"), 10.0, 1e-9);
    EXPECT_NEAR(result.node_voltages.at("aux"), 3.0, 1e-9);
    EXPECT_NEAR(result.source_currents.at("V1"), -0.007, 1e-12);
    EXPECT_NEAR(result.source_currents.at("V2"), 0.004, 1e-12);
}

TEST_CASE(dc_solver_rejects_circuits_without_ground) {
    const auto circuit = parse_or_fail(
        "V1 in ref 5\n"
        "R1 in ref 1k\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(dc_solver_rejects_unsupported_reactive_components) {
    const auto circuit = parse_or_fail(
        "V1 in 0 5\n"
        "C1 in 0 1u\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(dc_solver_detects_singular_matrix) {
    const auto circuit = parse_or_fail(
        "V1 in 0 5\n"
        "R1 out mid 1k\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(dc_solver_accepts_gnd_alias_for_ground) {
    const auto circuit = parse_or_fail(
        "V1 in GND 5\n"
        "R1 in out 1k\n"
        "R2 out GND 1k\n");

    const DCSolver solver;
    const auto result = solver.solve(circuit);

    EXPECT_TRUE(result.ok());
    EXPECT_NEAR(result.node_voltages.at("out"), 2.5, 1e-9);
}
