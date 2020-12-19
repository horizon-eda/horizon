#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlaceBoardPanel : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
                I::ROTATE,
        };
    }

private:
    const class IncludedBoard *inc_board = nullptr;
    class BoardPanel *temp = nullptr;
    void create_board(const Coordi &c);
};
} // namespace horizon
