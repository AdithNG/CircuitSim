#include "circuitsim/netlist_parser.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace circuitsim {
namespace {

std::string trim(std::string_view text) {
    std::size_t start = 0;
    while (start < text.size() && std::isspace(static_cast<unsigned char>(text[start])) != 0) {
        ++start;
    }

    std::size_t end = text.size();
    while (end > start && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
        --end;
    }

    return std::string(text.substr(start, end - start));
}

bool is_comment_or_empty(std::string_view line) {
    const std::string stripped = trim(line);
    return stripped.empty() || stripped.starts_with('*') || stripped.starts_with('#') || stripped.starts_with(';');
}

std::vector<std::string> split_tokens(std::string_view line) {
    std::vector<std::string> tokens;
    std::size_t index = 0;

    while (index < line.size()) {
        while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) != 0) {
            ++index;
        }

        if (index >= line.size()) {
            break;
        }

        const std::size_t start = index;
        while (index < line.size() && std::isspace(static_cast<unsigned char>(line[index])) == 0) {
            ++index;
        }

        tokens.emplace_back(line.substr(start, index - start));
    }

    return tokens;
}

ComponentType component_type_from_id(const std::string& id) {
    if (id.empty()) {
        throw std::invalid_argument("component identifier cannot be empty");
    }

    switch (static_cast<char>(std::toupper(static_cast<unsigned char>(id.front())))) {
        case 'R':
            return ComponentType::resistor;
        case 'C':
            return ComponentType::capacitor;
        case 'L':
            return ComponentType::inductor;
        case 'V':
            return ComponentType::voltage_source;
        case 'I':
            return ComponentType::current_source;
        default:
            throw std::invalid_argument("unsupported component type '" + std::string(1, id.front()) + "'");
    }
}

bool requires_positive_value(ComponentType type) {
    return type == ComponentType::resistor || type == ComponentType::capacitor || type == ComponentType::inductor;
}

void add_error(ParseResult& result, std::size_t line_number, std::string message) {
    result.errors.push_back(ParseError{line_number, std::move(message)});
}

void add_node_if_missing(std::vector<std::string>& nodes, const std::string& node) {
    if (std::find(nodes.begin(), nodes.end(), node) == nodes.end()) {
        nodes.push_back(node);
    }
}

double multiplier_for_suffix(std::string_view suffix) {
    std::string normalized;
    normalized.reserve(suffix.size());

    for (const char character : suffix) {
        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }

    if (normalized.empty()) {
        return 1.0;
    }
    if (normalized == "f") {
        return 1e-15;
    }
    if (normalized == "p") {
        return 1e-12;
    }
    if (normalized == "n") {
        return 1e-9;
    }
    if (normalized == "u") {
        return 1e-6;
    }
    if (normalized == "m") {
        return 1e-3;
    }
    if (normalized == "k") {
        return 1e3;
    }
    if (normalized == "meg") {
        return 1e6;
    }
    if (normalized == "g") {
        return 1e9;
    }
    if (normalized == "t") {
        return 1e12;
    }

    throw std::invalid_argument("unsupported numeric suffix '" + std::string(suffix) + "'");
}

}  // namespace

double parse_numeric_value(std::string_view token) {
    if (token.empty()) {
        throw std::invalid_argument("numeric value cannot be empty");
    }

    std::size_t suffix_start = token.size();
    while (suffix_start > 0) {
        const char character = token[suffix_start - 1];
        if (std::isalpha(static_cast<unsigned char>(character)) == 0) {
            break;
        }
        --suffix_start;
    }

    const std::string_view numeric_part = token.substr(0, suffix_start);
    const std::string_view suffix_part = token.substr(suffix_start);

    if (numeric_part.empty()) {
        throw std::invalid_argument("missing numeric value before suffix");
    }

    double base_value = 0.0;
    const auto* begin = numeric_part.data();
    const auto* end = numeric_part.data() + numeric_part.size();
    const auto [pointer, error_code] = std::from_chars(begin, end, base_value);

    if (error_code != std::errc{} || pointer != end) {
        throw std::invalid_argument("invalid numeric value '" + std::string(token) + "'");
    }

    const double multiplier = multiplier_for_suffix(suffix_part);
    const double value = base_value * multiplier;

    if (!std::isfinite(value)) {
        throw std::invalid_argument("numeric value is not finite");
    }

    return value;
}

ParseResult NetlistParser::parse_text(std::string_view text) const {
    ParseResult result;
    std::unordered_set<std::string> component_ids;

    std::size_t line_number = 0;
    std::size_t line_start = 0;

    while (line_start <= text.size()) {
        const std::size_t line_end = text.find('\n', line_start);
        const std::size_t raw_length = (line_end == std::string_view::npos) ? (text.size() - line_start) : (line_end - line_start);
        std::string line = trim(text.substr(line_start, raw_length));
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }

        ++line_number;

        if (!is_comment_or_empty(line)) {
            if (line.starts_with('.')) {
                if (line != ".end" && line != ".END") {
                    add_error(result, line_number, "unsupported directive '" + line + "'");
                }
            } else {
                const std::vector<std::string> tokens = split_tokens(line);
                if (tokens.size() != 4) {
                    add_error(result, line_number, "expected 4 tokens: <id> <node+> <node-> <value>");
                } else {
                    const std::string& id = tokens[0];
                    const std::string& node_positive = tokens[1];
                    const std::string& node_negative = tokens[2];
                    const std::string& value_token = tokens[3];

                    if (component_ids.contains(id)) {
                        add_error(result, line_number, "duplicate component identifier '" + id + "'");
                    } else if (node_positive == node_negative) {
                        add_error(result, line_number, "component must connect two distinct nodes");
                    } else {
                        try {
                            const ComponentType type = component_type_from_id(id);
                            const double value = parse_numeric_value(value_token);

                            if (requires_positive_value(type) && value <= 0.0) {
                                add_error(result, line_number, "passive component values must be positive");
                            } else {
                                component_ids.insert(id);
                                result.circuit.components.push_back(Component{
                                    .id = id,
                                    .type = type,
                                    .node_positive = node_positive,
                                    .node_negative = node_negative,
                                    .value = value,
                                    .line_number = line_number,
                                });

                                add_node_if_missing(result.circuit.nodes, node_positive);
                                add_node_if_missing(result.circuit.nodes, node_negative);
                            }
                        } catch (const std::invalid_argument& error) {
                            add_error(result, line_number, error.what());
                        }
                    }
                }
            }
        }

        if (line_end == std::string_view::npos) {
            break;
        }
        line_start = line_end + 1;
    }

    if (result.circuit.components.empty() && result.errors.empty()) {
        add_error(result, 0, "netlist did not contain any components");
    }

    return result;
}

}  // namespace circuitsim
