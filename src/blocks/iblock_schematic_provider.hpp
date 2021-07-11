#pragma once

namespace horizon {
class IBlockSchematicProvider {
public:
    virtual class Schematic &get_schematic(const class UUID &uu) = 0;
};
} // namespace horizon
