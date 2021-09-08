#pragma once
#include "idocument_schematic_block_symbol.hpp"
#include "util/uuid_vec.hpp"

namespace horizon {
class IDocumentSchematic : public virtual IDocumentSchematicBlockSymbol {
public:
    virtual class Schematic *get_current_schematic() = 0;
    virtual class Schematic *get_top_schematic() = 0;
    virtual class Sheet *get_sheet() = 0;
    virtual bool current_block_is_top() const = 0;

    virtual class Schematic &get_schematic_for_instance_path(const UUIDVec &path) = 0;
    virtual const UUIDVec &get_instance_path() const = 0;
    virtual bool in_hierarchy() const = 0;
};
} // namespace horizon
