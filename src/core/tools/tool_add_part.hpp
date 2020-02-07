#pragma once
#include "core/tool.hpp"
#include "tool_helper_map_symbol.hpp"
#include "tool_helper_move.hpp"

namespace horizon {
class ToolAddPart : public ToolHelperMapSymbol, public ToolHelperMove {
public:
    ToolAddPart(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    class ToolDataAddPart : public ToolData {
    public:
        ToolDataAddPart(const UUID &uu) : part_uuid(uu)
        {
        }
        const UUID part_uuid;
    };

private:
    unsigned int current_gate = 0;
    class SchematicSymbol *sym_current = nullptr;
    std::vector<const class Gate *> gates;
    class Component *comp = nullptr;
    void update_tip();
    UUID create_tag();
};
} // namespace horizon
