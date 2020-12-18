#pragma once
#include "clipboard.hpp"

namespace horizon {
class ClipboardPackage : public ClipboardBase {
public:
    ClipboardPackage(class IDocumentPackage &d) : doc(d)
    {
    }

protected:
    void serialize(json &j) override;

    IDocumentPackage &doc;
    IDocument &get_doc() override;
};
} // namespace horizon
