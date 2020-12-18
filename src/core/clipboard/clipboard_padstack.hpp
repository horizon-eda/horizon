#pragma once
#include "clipboard.hpp"

namespace horizon {
class ClipboardPadstack : public ClipboardBase {
public:
    ClipboardPadstack(class IDocumentPadstack &d) : doc(d)
    {
    }

protected:
    void serialize(json &j) override;

    IDocumentPadstack &doc;
    IDocument &get_doc() override;
};
} // namespace horizon
