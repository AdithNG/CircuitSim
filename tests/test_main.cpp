#include "test_framework.h"

#include <iostream>

int main() {
    std::size_t passed = 0;

    for (const auto& test_case : test::registry()) {
        try {
            test_case.body();
            ++passed;
            std::cout << "[PASS] " << test_case.name << '\n';
        } catch (const std::exception& error) {
            std::cerr << "[FAIL] " << test_case.name << ": " << error.what() << '\n';
            return 1;
        }
    }

    std::cout << "Executed " << passed << " tests successfully.\n";
    return 0;
}
