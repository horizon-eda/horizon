#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolLineLoopToPolygon : public ToolBase {
public:
    ToolLineLoopToPolygon(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    void remove_from_selection(ObjectType type, const UUID &uu);
};
} // namespace horizon
