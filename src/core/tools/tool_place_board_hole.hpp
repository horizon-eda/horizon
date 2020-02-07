#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlaceBoardHole : public ToolBase {
public:
    ToolPlaceBoardHole(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    const class Padstack *padstack = nullptr;
    class BoardHole *temp = nullptr;
    void create_hole(const Coordi &c);
};
} // namespace horizon
