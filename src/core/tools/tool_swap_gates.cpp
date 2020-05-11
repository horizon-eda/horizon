#include "tool_swap_gates.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "util/selection_util.hpp"
#include <iostream>

namespace horizon {

ToolSwapGates::ToolSwapGates(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

std::pair<class SchematicSymbol *, SchematicSymbol *> ToolSwapGates::get_symbols()
{
    std::pair<class SchematicSymbol *, SchematicSymbol *> r(nullptr, nullptr);
    if ((sel_count_type(selection, ObjectType::SCHEMATIC_SYMBOL) == 2) && selection.size() == 2) {
        auto it = selection.begin();
        r.first = doc.c->get_schematic_symbol(it->uuid);
        it++;
        r.second = doc.c->get_schematic_symbol(it->uuid);
        if (r.first->component == r.second->component
            && doc.c->get_block()->can_swap_gates(r.first->component->uuid, r.first->gate->uuid,
                                                  r.second->gate->uuid)) {
            return r;
        }
        else {
            return {nullptr, nullptr};
        }
    }
    return r;
}

bool ToolSwapGates::can_begin()
{
    if (!doc.c) {
        return false;
    }

    return get_symbols().first;
}

ToolResponse ToolSwapGates::begin(const ToolArgs &args)
{

    auto syms = get_symbols();
    doc.c->get_schematic()->swap_gates(syms.first->component->uuid, syms.first->gate->uuid, syms.second->gate->uuid);
    return ToolResponse::commit();
}

ToolResponse ToolSwapGates::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
