#pragma once
#include "tool_draw_polygon.hpp"
#include "tool_helper_edit_plane.hpp"

namespace horizon {

class ToolDrawPlane : public ToolDrawPolygon, private ToolHelperEditPlane {
public:
    using ToolDrawPolygon::ToolDrawPolygon;
    bool can_begin() override;

protected:
    bool done = false;

    ToolResponse commit() override;
    ToolResponse update(const ToolArgs &args) override;
};
} // namespace horizon
