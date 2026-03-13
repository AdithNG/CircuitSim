#pragma once

#include <exception>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace test {

struct TestCase {
    std::string name;
    std::function<void()> body;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline void register_test(std::string name, std::function<void()> body) {
    registry().push_back(TestCase{std::move(name), std::move(body)});
}

struct TestFailure : std::runtime_error {
    using std::runtime_error::runtime_error;
};

inline void fail(const std::string& message) {
    throw TestFailure(message);
}

}  // namespace test

#define TEST_CASE(name)                                                                 \
    static void name();                                                                 \
    namespace {                                                                         \
    const bool name##_registered = []() {                                               \
        ::test::register_test(#name, name);                                             \
        return true;                                                                    \
    }();                                                                                \
    }                                                                                   \
    static void name()

#define EXPECT_TRUE(condition)                                                          \
    do {                                                                                \
        if (!(condition)) {                                                             \
            ::test::fail(std::string("Expected true: ") + #condition);                  \
        }                                                                               \
    } while (false)

#define EXPECT_FALSE(condition) EXPECT_TRUE(!(condition))

#define EXPECT_EQ(actual, expected)                                                     \
    do {                                                                                \
        const auto& actual_value = (actual);                                            \
        const auto& expected_value = (expected);                                        \
        if (!(actual_value == expected_value)) {                                        \
            std::ostringstream stream;                                                  \
            stream << "Expected equality for " << #actual << " and " << #expected;      \
            ::test::fail(stream.str());                                                 \
        }                                                                               \
    } while (false)

#define EXPECT_NEAR(actual, expected, tolerance)                                        \
    do {                                                                                \
        const double actual_value = static_cast<double>(actual);                        \
        const double expected_value = static_cast<double>(expected);                    \
        const double tolerance_value = static_cast<double>(tolerance);                  \
        if ((actual_value < expected_value - tolerance_value) ||                        \
            (actual_value > expected_value + tolerance_value)) {                        \
            std::ostringstream stream;                                                  \
            stream << "Expected |" << #actual << " - " << #expected                     \
                   << "| <= " << #tolerance;                                            \
            ::test::fail(stream.str());                                                 \
        }                                                                               \
    } while (false)

#define EXPECT_THROW(statement)                                                         \
    do {                                                                                \
        bool threw = false;                                                             \
        try {                                                                           \
            statement;                                                                  \
        } catch (const std::exception&) {                                               \
            threw = true;                                                               \
        }                                                                               \
        if (!threw) {                                                                   \
            ::test::fail(std::string("Expected exception from: ") + #statement);        \
        }                                                                               \
    } while (false)
