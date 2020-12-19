#pragma once
#include "tool_place_junction.hpp"
#include "schematic/schematic_junction.hpp"

namespace horizon {
class ToolPlaceJunctionSchematic : public ToolPlaceJunctionT<SchematicJunction> {
public:
    using ToolPlaceJunctionT<SchematicJunction>::ToolPlaceJunctionT;

protected:
    bool junction_placed() override;
    void insert_junction() override;
    virtual bool check_line(class LineNet *li)
    {
        return true;
    }
};
} // namespace horizon
