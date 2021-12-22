#pragma once
#include "tool_helper_edit_plane.hpp"

namespace horizon {
class ToolEditPlane : public virtual ToolBase, private ToolHelperEditPlane {
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
        return {I::LMB, I::RMB};
    }

private:
    class Polygon *get_poly();
};
} // namespace horizon
