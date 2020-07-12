#pragma once
#include "core/tool.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {
class ToolDrawDimension : public ToolBase, public ToolHelperRestrict {
public:
    ToolDrawDimension(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::RESTRICT, I::DIMENSION_MODE, I::ENTER_DATUM,
        };
    }

private:
    class Dimension *temp = nullptr;
    void update_tip();

    enum class State { P0, P1, LABEL };
    State state = State::P0;
};
} // namespace horizon
