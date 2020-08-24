#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentDecal : public virtual IDocument {
public:
    virtual class Decal &get_decal() = 0;
};
} // namespace horizon
