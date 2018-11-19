#include "tool_helper_map_symbol.hpp"
#include "core_schematic.hpp"
#include "imp/imp_interface.hpp"
#include "util/sqlite.hpp"

namespace horizon {
SchematicSymbol *ToolHelperMapSymbol::map_symbol(Component *comp, const Gate *gate)
{
    UUID selected_symbol;

    std::string query = "SELECT symbols.uuid FROM symbols WHERE symbols.unit = ?";
    SQLite::Query q(core.r->m_pool->db, query);
    q.bind(1, gate->unit->uuid);
    int n = 0;
    while (q.step()) {
        selected_symbol = q.get<std::string>(0);
        n++;
    }

    bool r = false;
    if (n != 1) {
        if (placed_symbols.count(gate->unit->uuid) == 0) {
            std::tie(r, selected_symbol) = imp->dialogs.select_symbol(core.r->m_pool, gate->unit->uuid);
            placed_symbols.emplace(gate->unit->uuid, selected_symbol);
            if (!r) {
                return nullptr;
            }
        }
        else {
            selected_symbol = placed_symbols.at(gate->unit->uuid);
        }
    }

    const Symbol *sym = core.c->m_pool->get_symbol(selected_symbol);
    SchematicSymbol *schsym = core.c->insert_schematic_symbol(UUID::random(), sym);
    schsym->component = comp;
    schsym->gate = gate;
    core.c->get_sheet()->expand_symbols(*core.c->get_schematic());

    return schsym;
}
} // namespace horizon
