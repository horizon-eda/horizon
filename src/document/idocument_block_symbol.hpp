#pragma once
#include "idocument_schematic_block_symbol.hpp"

namespace horizon {
class IDocumentBlockSymbol : public virtual IDocumentSchematicBlockSymbol {
public:
    virtual class BlockSymbol &get_block_symbol() = 0;
};
} // namespace horizon
