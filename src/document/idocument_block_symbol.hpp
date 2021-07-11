#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentBlockSymbol : public virtual IDocument {
public:
    virtual class BlockSymbol &get_block_symbol() = 0;
    virtual bool get_block_symbol_mode() const = 0;
};
} // namespace horizon
