#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolDrawPolygonCircle : public ToolBase {
public:
    ToolDrawPolygonCircle(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

private:
    Coordi first_pos;
    Coordi second_pos;

    int step = 0;
    class Polygon *temp = nullptr;

    void update_polygon();
    void update_tip();
};
} // namespace horizon
