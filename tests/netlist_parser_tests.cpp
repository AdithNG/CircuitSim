#include "circuitsim/netlist_parser.h"
#include "test_framework.h"

#include <string>

using circuitsim::ComponentType;
using circuitsim::NetlistParser;
using circuitsim::parse_numeric_value;

namespace {

std::string simple_netlist() {
    return "V1 in 0 5\nR1 in out 1k\nR2 out 0 2k\n.end\n";
}

}  // namespace

TEST_CASE(parse_numeric_value_supports_plain_numbers) {
    EXPECT_NEAR(parse_numeric_value("10"), 10.0, 1e-12);
    EXPECT_NEAR(parse_numeric_value("-2.5"), -2.5, 1e-12);
    EXPECT_NEAR(parse_numeric_value("3.125"), 3.125, 1e-12);
}

TEST_CASE(parse_numeric_value_supports_engineering_suffixes) {
    EXPECT_NEAR(parse_numeric_value("10k"), 10000.0, 1e-12);
    EXPECT_NEAR(parse_numeric_value("3u"), 3e-6, 1e-18);
    EXPECT_NEAR(parse_numeric_value("7meg"), 7e6, 1e-6);
    EXPECT_NEAR(parse_numeric_value("8G"), 8e9, 1e-3);
    EXPECT_NEAR(parse_numeric_value("9p"), 9e-12, 1e-18);
}

TEST_CASE(parse_numeric_value_rejects_invalid_inputs) {
    EXPECT_THROW(static_cast<void>(parse_numeric_value("")));
    EXPECT_THROW(static_cast<void>(parse_numeric_value("k")));
    EXPECT_THROW(static_cast<void>(parse_numeric_value("1x")));
    EXPECT_THROW(static_cast<void>(parse_numeric_value("abc")));
}

TEST_CASE(parser_accepts_basic_linear_netlist) {
    const NetlistParser parser;
    const auto result = parser.parse_text(simple_netlist());

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.components.size(), 3U);
    EXPECT_EQ(result.circuit.nodes.size(), 3U);
    EXPECT_EQ(result.circuit.components[0].id, "V1");
    EXPECT_EQ(result.circuit.components[1].type, ComponentType::resistor);
    EXPECT_EQ(result.circuit.components[2].node_positive, "out");
}

TEST_CASE(parser_handles_comments_and_blank_lines) {
    const NetlistParser parser;
    const std::string netlist =
        "* divider example\n"
        "\n"
        "V1 in 0 5\n"
        "; output load\n"
        "R1 in out 1k\n"
        "# ground path\n"
        "R2 out 0 2k\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.components.size(), 3U);
}

TEST_CASE(parser_rejects_duplicate_component_ids) {
    const NetlistParser parser;
    const std::string netlist =
        "R1 in out 1k\n"
        "R1 out 0 2k\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
    EXPECT_EQ(result.errors[0].line_number, 2U);
}

TEST_CASE(parser_rejects_unsupported_component_type) {
    const NetlistParser parser;
    const std::string netlist = "Q1 c b e 1\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
    EXPECT_EQ(result.errors[0].line_number, 1U);
}

TEST_CASE(parser_rejects_bad_token_counts) {
    const NetlistParser parser;
    const std::string netlist = "R1 in out\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(parser_rejects_identical_nodes_for_component) {
    const NetlistParser parser;
    const std::string netlist = "R1 out out 10\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(parser_rejects_non_positive_passive_values) {
    const NetlistParser parser;
    const std::string netlist =
        "R1 in out 0\n"
        "C1 out 0 -1u\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 2U);
}

TEST_CASE(parser_allows_negative_source_values) {
    const NetlistParser parser;
    const std::string netlist =
        "V1 in 0 -5\n"
        "I1 in 0 -1m\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.components.size(), 2U);
}

TEST_CASE(parser_rejects_unsupported_directives) {
    const NetlistParser parser;
    const std::string netlist =
        ".tran 1n 10n\n"
        "R1 in out 1k\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
}

TEST_CASE(parser_accepts_end_directive) {
    const NetlistParser parser;
    const auto result = parser.parse_text(simple_netlist());

    EXPECT_TRUE(result.ok());
}

TEST_CASE(parser_reports_empty_netlist) {
    const NetlistParser parser;
    const auto result = parser.parse_text("  \n ; comment only\n");

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 1U);
    EXPECT_EQ(result.errors[0].line_number, 0U);
}

TEST_CASE(parser_collects_unique_nodes) {
    const NetlistParser parser;
    const std::string netlist =
        "V1 in 0 5\n"
        "R1 in mid 1k\n"
        "R2 mid 0 2k\n"
        "R3 mid out 500\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.nodes.size(), 4U);
    EXPECT_EQ(result.circuit.nodes[0], "in");
    EXPECT_EQ(result.circuit.nodes[1], "0");
    EXPECT_EQ(result.circuit.nodes[2], "mid");
    EXPECT_EQ(result.circuit.nodes[3], "out");
}

TEST_CASE(parser_preserves_component_line_numbers) {
    const NetlistParser parser;
    const std::string netlist =
        "\n"
        "V1 in 0 5\n"
        "R1 in out 1k\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.components[0].line_number, 2U);
    EXPECT_EQ(result.circuit.components[1].line_number, 3U);
}

TEST_CASE(parser_supports_case_insensitive_component_prefixes) {
    const NetlistParser parser;
    const std::string netlist =
        "r1 in out 1k\n"
        "v1 in 0 5\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.components[0].type, ComponentType::resistor);
    EXPECT_EQ(result.circuit.components[1].type, ComponentType::voltage_source);
}

TEST_CASE(parser_accepts_windows_line_endings) {
    const NetlistParser parser;
    const std::string netlist = "V1 in 0 5\r\nR1 in out 1k\r\nR2 out 0 2k\r\n.END\r\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.components.size(), 3U);
}

TEST_CASE(parser_collects_multiple_errors_in_one_pass) {
    const NetlistParser parser;
    const std::string netlist =
        "R1 in in 10\n"
        "Q1 c b e 1\n"
        ".tran 1n 10n\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_FALSE(result.ok());
    EXPECT_EQ(result.errors.size(), 3U);
    EXPECT_EQ(result.errors[0].line_number, 1U);
    EXPECT_EQ(result.errors[1].line_number, 2U);
    EXPECT_EQ(result.errors[2].line_number, 3U);
}

TEST_CASE(parser_treats_ground_as_a_regular_named_node) {
    const NetlistParser parser;
    const std::string netlist =
        "V1 in GND 5\n"
        "R1 in out 1k\n"
        "R2 out GND 2k\n";

    const auto result = parser.parse_text(netlist);

    EXPECT_TRUE(result.ok());
    EXPECT_EQ(result.circuit.nodes.size(), 3U);
    EXPECT_EQ(result.circuit.nodes[1], "GND");
}
