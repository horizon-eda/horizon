#pragma once
#include "block/component.hpp"
#include "core/tool.hpp"
#include <forward_list>

namespace horizon {

class ToolAssignPart : public ToolBase {
public:
    ToolAssignPart(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return true;
    }

private:
    const class Entity *get_entity();
    Component *comp;
};
} // namespace horizon
