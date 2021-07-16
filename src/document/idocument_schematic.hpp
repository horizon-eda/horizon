#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentSchematic : public virtual IDocument {
public:
    virtual class Schematic *get_schematic() = 0;
    virtual class Sheet *get_sheet() = 0;
};
} // namespace horizon
