#include "tool_helper_map_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "canvas/canvas_gl.hpp"
#include "imp/imp_interface.hpp"
#include "util/sqlite.hpp"

namespace horizon {
const Symbol *ToolHelperMapSymbol::get_symbol_for_unit(const UUID &unit_uu, bool *auto_selected)
{
    UUID selected_symbol;

    std::string query = "SELECT symbols.uuid FROM symbols WHERE symbols.unit = ?";
    SQLite::Query q(doc.r->get_pool()->db, query);
    q.bind(1, unit_uu);
    int n = 0;
    while (q.step()) {
        selected_symbol = q.get<std::string>(0);
        n++;
    }
    if (auto_selected)
        *auto_selected = false;
    bool r = false;
    if (n != 1) {
        std::tie(r, selected_symbol) = imp->dialogs.select_symbol(doc.r->get_pool(), unit_uu);
        if (!r) {
            return nullptr;
        }
    }
    else if (n == 1) {
        if (auto_selected)
            *auto_selected = true;
    }
    else if (n == 0) {
        return nullptr;
    }

    return doc.c->get_pool()->get_symbol(selected_symbol);
}


SchematicSymbol *ToolHelperMapSymbol::map_symbol(Component *comp, const Gate *gate)
{
    const Symbol *sym = get_symbol_for_unit(gate->unit->uuid);
    if (!sym)
        return nullptr;
    SchematicSymbol *schsym = doc.c->insert_schematic_symbol(UUID::random(), sym);
    schsym->component = comp;
    schsym->gate = gate;
    auto bb = schsym->symbol.get_bbox(true);
    auto sz = bb.second - bb.first;
    imp->get_canvas()->ensure_min_size(sz.x * 1.5, sz.y * 1.5);

    doc.c->get_sheet()->expand_symbol(schsym->uuid, *doc.c->get_schematic());

    return schsym;
}
} // namespace horizon
