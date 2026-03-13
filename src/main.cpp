#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <string>

#include "circuitsim/diagnostics.h"
#include "circuitsim/dc_solver.h"
#include "circuitsim/netlist_parser.h"
#include "circuitsim/transient_solver.h"

namespace {

std::map<std::string, double> sort_scalars(const std::unordered_map<std::string, double>& values) {
    return std::map<std::string, double>(values.begin(), values.end());
}

std::map<std::string, std::vector<double>> sort_series(
    const std::unordered_map<std::string, std::vector<double>>& values
) {
    return std::map<std::string, std::vector<double>>(values.begin(), values.end());
}

void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  circuitsim_cli <netlist-file>\n";
    std::cerr << "  circuitsim_cli tran <netlist-file> <time-step> <stop-time>\n";
}

void print_diagnostics(const std::vector<circuitsim::DiagnosticMessage>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        const char* severity =
            diagnostic.severity == circuitsim::DiagnosticSeverity::error ? "error" : "warning";
        std::cerr << "Diagnostic " << severity << " [" << diagnostic.code << "]: "
                  << diagnostic.message << '\n';
    }
}

}  // namespace

int main(int argc, char** argv) {
    const bool transient_mode = argc == 5 && std::string(argv[1]) == "tran";
    const bool dc_mode = argc == 2;

    if (!dc_mode && !transient_mode) {
        print_usage();
        return 1;
    }

    const char* input_path = transient_mode ? argv[2] : argv[1];
    std::ifstream input(input_path, std::ios::binary);
    if (!input) {
        std::cerr << "Failed to open netlist file: " << input_path << '\n';
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

    if (dc_mode) {
        const circuitsim::DCSolver solver;
        const auto solve_result = solver.solve(parse_result.circuit);
        print_diagnostics(solve_result.diagnostics);
        if (!solve_result.ok()) {
            for (const auto& error : solve_result.errors) {
                std::cerr << "Solve error: " << error.message << '\n';
            }
            return 1;
        }

        std::cout << std::fixed << std::setprecision(6);
        std::cout << "Node voltages:\n";
        for (const auto& [node, voltage] : sort_scalars(solve_result.node_voltages)) {
            std::cout << "  " << node << " = " << voltage << " V\n";
        }

        if (!solve_result.source_currents.empty()) {
            std::cout << "Source currents:\n";
            for (const auto& [source, current] : sort_scalars(solve_result.source_currents)) {
                std::cout << "  " << source << " = " << current << " A\n";
            }
        }
        return 0;
    }

    const circuitsim::TransientAnalysisConfig config{
        .time_step = std::stod(argv[3]),
        .stop_time = std::stod(argv[4]),
    };

    const circuitsim::TransientSolver solver;
    const auto solve_result = solver.solve(parse_result.circuit, config);
    print_diagnostics(solve_result.diagnostics);
    if (!solve_result.ok()) {
        for (const auto& error : solve_result.errors) {
            std::cerr << "Solve error: " << error.message << '\n';
        }
        return 1;
    }

    std::cout << std::fixed << std::setprecision(6);
    std::cout << "time";
    for (const auto& [node, _] : sort_series(solve_result.node_voltages)) {
        std::cout << "," << node;
    }
    std::cout << '\n';

    const auto sorted_node_series = sort_series(solve_result.node_voltages);
    for (std::size_t index = 0; index < solve_result.time_points.size(); ++index) {
        std::cout << solve_result.time_points[index];
        for (const auto& [node, samples] : sorted_node_series) {
            (void)node;
            std::cout << "," << samples[index];
        }
        std::cout << '\n';
    }

    return 0;
}
