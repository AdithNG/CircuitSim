#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <string>

#include "circuitsim/dc_solver.h"
#include "circuitsim/netlist_parser.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: circuitsim_cli <netlist-file>\n";
        return 1;
    }

    std::ifstream input(argv[1], std::ios::binary);
    if (!input) {
        std::cerr << "Failed to open netlist file: " << argv[1] << '\n';
        return 1;
    }

    const std::string text((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());

    const circuitsim::NetlistParser parser;
    const auto parse_result = parser.parse_text(text);
    if (!parse_result.ok()) {
        for (const auto& error : parse_result.errors) {
            std::cerr << "Parse error";
            if (error.line_number != 0) {
                std::cerr << " on line " << error.line_number;
            }
            std::cerr << ": " << error.message << '\n';
        }
        return 1;
    }

    const circuitsim::DCSolver solver;
    const auto solve_result = solver.solve(parse_result.circuit);
    if (!solve_result.ok()) {
        for (const auto& error : solve_result.errors) {
            std::cerr << "Solve error: " << error.message << '\n';
        }
        return 1;
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "Node voltages:\n";
    for (const auto& [node, voltage] : solve_result.node_voltages) {
        std::cout << "  " << node << " = " << voltage << " V\n";
    }

    if (!solve_result.source_currents.empty()) {
        std::cout << "Source currents:\n";
        for (const auto& [source, current] : solve_result.source_currents) {
            std::cout << "  " << source << " = " << current << " A\n";
        }
    }

    return 0;
}
