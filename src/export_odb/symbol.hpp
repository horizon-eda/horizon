#pragma once
#include "features.hpp"
#include <string>

namespace horizon {
class Padstack;
class TreeWriter;
class LayerProvider;
} // namespace horizon

namespace horizon::ODB {

class Symbol {
public:
    Symbol(const Padstack &ps, int layer, const LayerProvider &lprv);

    std::string name;
    Features features;

    void write(TreeWriter &writer) const;
};
} // namespace horizon::ODB
