#pragma once
#include "core/tool.hpp"
#include "tool_helper_map_symbol.hpp"
#include "tool_helper_move.hpp"
#include <list>

namespace horizon {
class ToolMapSymbol : public ToolHelperMapSymbol, public ToolHelperMove {
public:
    ToolMapSymbol(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    class ToolDataMapSymbol : public ToolData {
    public:
        ToolDataMapSymbol(const std::vector<UUIDPath<2>> &g) : gates(g)
        {
        }
        const std::vector<UUIDPath<2>> gates;
    };

private:
    void update_tip();
    std::map<UUIDPath<2>, std::string> gates_out;
    class SchematicSymbol *sym_current = nullptr;
    std::list<UUIDPath<2>> gates_from_data;
    bool data_mode = false;
};
} // namespace horizon
