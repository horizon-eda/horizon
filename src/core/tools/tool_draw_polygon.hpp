#pragma once
#include "common/polygon.hpp"
#include "core/tool.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {

class ToolDrawPolygon : public ToolBase, public ToolHelperRestrict {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {I::LMB, I::CANCEL, I::RMB, I::FLIP_ARC, I::TOGGLE_ARC, I::RESTRICT, I::TOGGLE_DEG45_RESTRICT};
    }

protected:
    Polygon *temp = nullptr;
    virtual ToolResponse commit();

private:
    Polygon::Vertex *vertex = nullptr;
    Polygon::Vertex *last_vertex = nullptr;
    enum class ArcMode { OFF, NEXT, CURRENT };
    ArcMode arc_mode = ArcMode::OFF;
    void update_tip();
    void update_vertex(const Coordi &c);
    void set_snap_filter();
    void append_vertex(const Coordi &c);
};
} // namespace horizon
