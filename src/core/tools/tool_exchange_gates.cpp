#include "tool_exchange_gates.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "util/selection_util.hpp"
#include "util/util.hpp"

namespace horizon {

std::pair<SchematicSymbol *, SchematicSymbol *> ToolExchangeGates::get_symbols()
{
    std::pair<SchematicSymbol *, SchematicSymbol *> r(nullptr, nullptr);
    if ((sel_count_type(selection, ObjectType::SCHEMATIC_SYMBOL) == 2) && selection.size() == 2) {
        auto it = selection.begin();
        r.first = doc.c->get_schematic_symbol(it->uuid);
        it++;
        r.second = doc.c->get_schematic_symbol(it->uuid);
        if (r.first->component != r.second->component && (r.first->gate->unit->uuid == r.second->gate->unit->uuid))
            return r;
        else
            return {nullptr, nullptr};
    }
    return r;
}

bool ToolExchangeGates::can_begin()
{
    if (!doc.c) {
        return false;
    }

    return get_symbols().first;
}

using Connections = decltype(Component::connections);

static void erase_conns(SchematicSymbol &sym)
{
    map_erase_if(sym.component->connections, [&sym](const auto &it) { return it.first.at(0) == sym.gate->uuid; });
}

static auto extract_conns(SchematicSymbol &sym)
{
    Connections conns;
    for (const auto &[k, v] : sym.component->connections) {
        if (k.at(0) == sym.gate->uuid)
            conns.emplace(k, v);
    }
    return conns;
}

static void apply_conns(SchematicSymbol &to, const Connections &from)
{
    for (const auto &[k, v] : from) {
        to.component->connections.emplace(std::piecewise_construct, std::forward_as_tuple(to.gate->uuid, k.at(1)),
                                          std::forward_as_tuple(v));
    }
}

ToolResponse ToolExchangeGates::begin(const ToolArgs &args)
{
    auto syms = get_symbols();

    // swap connections in block
    const auto conn1 = extract_conns(*syms.first);
    const auto conn2 = extract_conns(*syms.second);
    erase_conns(*syms.first);
    erase_conns(*syms.second);
    apply_conns(*syms.first, conn2);
    apply_conns(*syms.second, conn1);

    // swap symbol mapping
    std::swap(syms.first->component, syms.second->component);
    std::swap(syms.first->gate, syms.second->gate);

    return ToolResponse::commit();
}

ToolResponse ToolExchangeGates::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
