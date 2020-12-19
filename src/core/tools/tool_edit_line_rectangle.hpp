#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolEditLineRectangle : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }
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
    std::array<class Junction *, 4> junctions;

    void update_junctions(const Coordi &c);
};
} // namespace horizon
