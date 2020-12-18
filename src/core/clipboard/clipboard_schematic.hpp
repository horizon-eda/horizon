#pragma once
#include "clipboard.hpp"

namespace horizon {
class ClipboardSchematic : public ClipboardBase {
public:
    ClipboardSchematic(class IDocumentSchematic &d) : doc(d)
    {
    }

protected:
    void expand_selection() override;
    void serialize(json &j) override;

    IDocumentSchematic &doc;
    IDocument &get_doc() override;
};
} // namespace horizon
