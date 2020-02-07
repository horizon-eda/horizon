#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentFrame : public virtual IDocument {
public:
    virtual class Frame *get_frame() = 0;
};
} // namespace horizon
