#pragma once
#include "idocument.hpp"

namespace horizon {
class IDocumentBoard : public virtual IDocument {
public:
    virtual class Board *get_board() = 0;
    virtual ViaPadstackProvider *get_via_padstack_provider() = 0;
};
} // namespace horizon
