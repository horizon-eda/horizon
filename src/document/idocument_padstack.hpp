#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentPadstack : public virtual IDocument {
public:
    virtual class Padstack *get_padstack() = 0;
};
} // namespace horizon
