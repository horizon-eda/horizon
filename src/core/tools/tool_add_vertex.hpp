#pragma once
#include "common/polygon.hpp"
#include "core/tool.hpp"
#include <forward_list>

namespace horizon {

class ToolAddVertex : public ToolBase {
public:
    ToolAddVertex(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    class Polygon *poly = nullptr;
    Polygon::Vertex *vertex = nullptr;
};
} // namespace horizon
