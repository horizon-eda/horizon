#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlaceBoardHole : public ToolBase {
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
        };
    }

private:
    const class Padstack *padstack = nullptr;
    class BoardHole *temp = nullptr;
    void create_hole(const Coordi &c);
};
} // namespace horizon
