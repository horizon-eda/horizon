#pragma once
#include "clipboard.hpp"

namespace horizon {
class ClipboardBoard : public ClipboardBase {
public:
    ClipboardBoard(class IDocumentBoard &d) : doc(d)
    {
    }

protected:
    void expand_selection() override;
    void serialize(json &j) override;

    IDocumentBoard &doc;
    IDocument &get_doc() override;
};
} // namespace horizon
