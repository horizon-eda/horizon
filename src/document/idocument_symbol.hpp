#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentSymbol : public virtual IDocument {
public:
    virtual class Symbol *get_symbol() = 0;

    virtual class SymbolPin *get_symbol_pin(const class UUID &uu) = 0;
    virtual SymbolPin *insert_symbol_pin(const UUID &uu) = 0;
    virtual void delete_symbol_pin(const UUID &uu) = 0;
    virtual std::vector<const class Pin *> get_pins() = 0;
};
} // namespace horizon
