#pragma once
#include "core/tool.hpp"
#include "package/pad.hpp"

namespace horizon {
class ToolPlacePad : public ToolBase {
public:
    ToolPlacePad(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    const Padstack *padstack = nullptr;
    Pad *temp = nullptr;
    void create_pad(const Coordi &c);
};
} // namespace horizon
