#pragma once
#include "common/polygon.hpp"
#include "core/tool.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {

class ToolDrawPolygon : public ToolBase, public ToolHelperRestrict {
public:
    ToolDrawPolygon(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    Polygon *temp = nullptr;
    Polygon::Vertex *vertex = nullptr;
    Polygon::Vertex *last_vertex = nullptr;
    enum class ArcMode { OFF, NEXT, CURRENT };
    ArcMode arc_mode = ArcMode::OFF;
    void update_tip();
    void update_vertex(const Coordi &c);
    void set_snap_filter();
};
} // namespace horizon
