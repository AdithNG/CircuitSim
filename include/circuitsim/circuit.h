#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace circuitsim {

enum class ComponentType {
    resistor,
    capacitor,
    inductor,
    voltage_source,
    current_source,
};

struct Component {
    std::string id;
    ComponentType type;
    std::string node_positive;
    std::string node_negative;
    double value = 0.0;
    std::size_t line_number = 0;
};

struct Circuit {
    std::vector<Component> components;
    std::vector<std::string> nodes;
};

}  // namespace circuitsim
