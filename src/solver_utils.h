#pragma once

#include <algorithm>
#include <cctype>
#include <cmath>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace circuitsim {

constexpr double kPivotTolerance = 1e-12;

inline bool is_ground_node(const std::string& node_name) {
    if (node_name == "0") {
        return true;
    }

    std::string normalized;
    normalized.reserve(node_name.size());
    for (const char character : node_name) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }

    return normalized == "gnd";
}

inline std::vector<double> solve_linear_system(
    std::vector<std::vector<double>> matrix,
    std::vector<double> rhs
) {
    const std::size_t size = matrix.size();
    for (std::size_t pivot = 0; pivot < size; ++pivot) {
        std::size_t best_row = pivot;
        double best_value = std::fabs(matrix[pivot][pivot]);

        for (std::size_t candidate = pivot + 1; candidate < size; ++candidate) {
            const double candidate_value = std::fabs(matrix[candidate][pivot]);
            if (candidate_value > best_value) {
                best_value = candidate_value;
                best_row = candidate;
            }
        }

        if (best_value < kPivotTolerance) {
            throw std::runtime_error("matrix is singular");
        }

        if (best_row != pivot) {
            std::swap(matrix[pivot], matrix[best_row]);
            std::swap(rhs[pivot], rhs[best_row]);
        }

        const double pivot_value = matrix[pivot][pivot];
        for (std::size_t row = pivot + 1; row < size; ++row) {
            const double factor = matrix[row][pivot] / pivot_value;
            if (std::fabs(factor) < kPivotTolerance) {
                continue;
            }

            for (std::size_t column = pivot; column < size; ++column) {
                matrix[row][column] -= factor * matrix[pivot][column];
            }
            rhs[row] -= factor * rhs[pivot];
        }
    }

    std::vector<double> solution(size, 0.0);
    for (std::size_t row = size; row-- > 0;) {
        double value = rhs[row];
        for (std::size_t column = row + 1; column < size; ++column) {
            value -= matrix[row][column] * solution[column];
        }
        solution[row] = value / matrix[row][row];
    }

    return solution;
}

}  // namespace circuitsim
