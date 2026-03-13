#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <string>
#include <sstream>
#include <vector>

#include "circuitsim/ac_solver.h"
#include "circuitsim/diagnostics.h"
#include "circuitsim/dc_solver.h"
#include "circuitsim/netlist_parser.h"
#include "circuitsim/transient_solver.h"

namespace {

std::map<std::string, double> sort_scalars(const std::unordered_map<std::string, double>& values) {
    return std::map<std::string, double>(values.begin(), values.end());
}

template <typename T>
std::map<std::string, std::vector<T>> sort_series(
    const std::unordered_map<std::string, std::vector<T>>& values
) {
    return std::map<std::string, std::vector<T>>(values.begin(), values.end());
}

void print_usage() {
    std::cerr << "Usage:\n";
    std::cerr << "  circuitsim_cli <netlist-file>\n";
    std::cerr << "  circuitsim_cli tran <netlist-file> <time-step> <stop-time>\n";
    std::cerr << "  circuitsim_cli ac <netlist-file> <freq1[,freq2,...]>\n";
}

void print_diagnostics(const std::vector<circuitsim::DiagnosticMessage>& diagnostics) {
    for (const auto& diagnostic : diagnostics) {
        const char* severity =
            diagnostic.severity == circuitsim::DiagnosticSeverity::error ? "error" : "warning";
        std::cerr << "Diagnostic " << severity << " [" << diagnostic.code << "]: "
                  << diagnostic.message << '\n';
    }
}

void print_summary(const circuitsim::AnalysisSummary& summary) {
    std::cout << "Summary:\n";
    std::cout << "  analysis = " << summary.analysis_type << '\n';
    std::cout << "  nodes = " << summary.node_count << '\n';
    std::cout << "  components = " << summary.component_count << '\n';
    std::cout << "  resistors = " << summary.resistor_count << '\n';
    std::cout << "  capacitors = " << summary.capacitor_count << '\n';
    std::cout << "  inductors = " << summary.inductor_count << '\n';
    std::cout << "  voltage_sources = " << summary.voltage_source_count << '\n';
    std::cout << "  current_sources = " << summary.current_source_count << '\n';
    std::cout << "  samples = " << summary.sample_count << '\n';
    if (summary.analysis_type == "transient") {
        std::cout << "  time_step = " << summary.time_step << '\n';
        std::cout << "  stop_time = " << summary.stop_time << '\n';
    }
}

std::vector<double> parse_frequency_list(const std::string& text) {
    std::vector<double> frequencies;
    std::stringstream stream(text);
    std::string item;
    while (std::getline(stream, item, ',')) {
        if (!item.empty()) {
            frequencies.push_back(std::stod(item));
        }
    }
    return frequencies;
}

}  // namespace

int main(int argc, char** argv) {
    const bool transient_mode = argc == 5 && std::string(argv[1]) == "tran";
    const bool ac_mode = argc == 4 && std::string(argv[1]) == "ac";
    const bool dc_mode = argc == 2;

    if (!dc_mode && !transient_mode && !ac_mode) {
        print_usage();
        return 1;
    }

    const char* input_path = (transient_mode || ac_mode) ? argv[2] : argv[1];
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
        print_summary(solve_result.summary);
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

    if (ac_mode) {
        const circuitsim::ACSolver solver;
        const auto solve_result = solver.solve(parse_result.circuit, parse_frequency_list(argv[3]));
        print_diagnostics(solve_result.diagnostics);
        if (!solve_result.ok()) {
            for (const auto& error : solve_result.errors) {
                std::cerr << "Solve error: " << error.message << '\n';
            }
            return 1;
        }

        std::cout << std::fixed << std::setprecision(6);
        print_summary(solve_result.summary);
        std::cout << "frequency_hz,node,magnitude,phase_rad\n";
        const auto sorted_nodes = sort_series(solve_result.node_voltages);
        for (std::size_t freq_index = 0; freq_index < solve_result.frequencies_hz.size(); ++freq_index) {
            for (const auto& [node, samples] : sorted_nodes) {
                const auto value = samples[freq_index];
                std::cout << solve_result.frequencies_hz[freq_index]
                          << "," << node
                          << "," << std::abs(value)
                          << "," << std::arg(value)
                          << '\n';
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
    print_summary(solve_result.summary);
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
