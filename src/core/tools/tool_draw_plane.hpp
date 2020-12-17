#pragma once
#include "tool_draw_polygon.hpp"

namespace horizon {

class ToolDrawPlane : public ToolDrawPolygon {
public:
    using ToolDrawPolygon::ToolDrawPolygon;
    bool can_begin()  override;

protected:
    ToolResponse commit() override;
};
} // namespace horizon