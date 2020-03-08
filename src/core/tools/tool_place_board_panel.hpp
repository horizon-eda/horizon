#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlaceBoardPanel : public ToolBase {
public:
    ToolPlaceBoardPanel(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    const class IncludedBoard *inc_board = nullptr;
    class BoardPanel *temp = nullptr;
    void create_board(const Coordi &c);
};
} // namespace horizon
