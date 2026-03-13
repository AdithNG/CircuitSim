#pragma once

#include <string>
#include <string_view>
#include <vector>

#include "circuitsim/circuit.h"

namespace circuitsim {

struct ParseError {
    std::size_t line_number = 0;
    std::string message;
};

struct ParseResult {
    Circuit circuit;
    std::vector<ParseError> errors;

    [[nodiscard]] bool ok() const {
        return errors.empty();
    }
};

class NetlistParser {
public:
    [[nodiscard]] ParseResult parse_text(std::string_view text) const;
};

[[nodiscard]] double parse_numeric_value(std::string_view token);

}  // namespace circuitsim
