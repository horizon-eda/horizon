#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentSchematicBlockSymbol : public virtual IDocument {
public:
    virtual bool get_block_symbol_mode() const = 0;
    virtual class Block *get_current_block() = 0;
    virtual class BlocksSchematic &get_blocks() = 0;
};
} // namespace horizon
