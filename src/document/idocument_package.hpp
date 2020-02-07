#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentPackage : public virtual IDocument {
public:
    virtual class Package *get_package() = 0;
};
} // namespace horizon
