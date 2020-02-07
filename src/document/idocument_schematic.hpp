#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentSchematic : public virtual IDocument {
public:
    virtual class Schematic *get_schematic() = 0;
    virtual class Sheet *get_sheet() = 0;

    virtual class SchematicSymbol *get_schematic_symbol(const UUID &uu) = 0;
    virtual class SchematicSymbol *insert_schematic_symbol(const class UUID &uu, const class Symbol *sym) = 0;
    virtual void delete_schematic_symbol(const UUID &uu) = 0;

    virtual class LineNet *insert_line_net(const UUID &uu) = 0;
    virtual void delete_line_net(const UUID &uu) = 0;

    virtual std::vector<class LineNet *> get_net_lines() = 0;
    virtual std::vector<class NetLabel *> get_net_labels() = 0;
};
} // namespace horizon
