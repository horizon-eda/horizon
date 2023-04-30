#pragma once
#include "util/uuid.hpp"
#include "parameter/set.hpp"
#include "util/layer_range.hpp"

namespace horizon {
class ViaDefinition {
public:
    ViaDefinition(const UUID &uu);
    ViaDefinition(const UUID &uu, const json &j);
    json serialize() const;

    UUID uuid;

    std::string name;
    UUID padstack;
    ParameterSet parameters;
    LayerRange span;
};
} // namespace horizon
