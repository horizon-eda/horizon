#include "tool_helper_map_symbol.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "canvas/canvas_gl.hpp"
#include "imp/imp_interface.hpp"
#include "util/sqlite.hpp"
#include "pool/ipool.hpp"
#include "core/tool_id.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
const Symbol *ToolHelperMapSymbol::get_symbol_for_unit(const UUID &unit_uu, bool *auto_selected)
{
    UUID selected_symbol;

    std::string query = "SELECT symbols.uuid FROM symbols WHERE symbols.unit = ?";
    SQLite::Query q(doc.r->get_pool().get_db(), query);
    q.bind(1, unit_uu);
    int n = 0;
    while (q.step()) {
        selected_symbol = q.get<std::string>(0);
        n++;
    }
    if (auto_selected)
        *auto_selected = false;
    if (n != 1) {
        if (auto r = imp->dialogs.select_symbol(doc.r->get_pool(), unit_uu)) {
            selected_symbol = *r;
        }
        else {
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
    settings.selected_symbols[unit_uu] = selected_symbol;

    return doc.c->get_pool().get_symbol(selected_symbol);
}


SchematicSymbol *ToolHelperMapSymbol::map_symbol(Component *comp, const Gate *gate)
{
    const Symbol *sym = nullptr;
    if (settings.selected_symbols.count(gate->unit->uuid))
        sym = doc.c->get_pool().get_symbol(settings.selected_symbols.at(gate->unit->uuid));
    if (!sym)
        sym = get_symbol_for_unit(gate->unit->uuid);
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

void ToolHelperMapSymbol::change_symbol(SchematicSymbol *schsym)
{
    bool auto_selected = false;
    auto gate = schsym->gate;
    auto new_sym = get_symbol_for_unit(gate->unit->uuid, &auto_selected);
    if (new_sym) {
        if (auto_selected) {
            imp->tool_bar_flash("There's only one symbol for this unit");
        }
        else {
            settings.selected_symbols[gate->unit->uuid] = new_sym->uuid;
            schsym->pool_symbol = new_sym;
            doc.c->get_sheet()->expand_symbol(schsym->uuid, *doc.c->get_schematic());
        }
    }
}

ToolID ToolHelperMapSymbol::get_tool_id_for_settings() const
{
    return ToolID::MAP_SYMBOL;
}

json ToolHelperMapSymbol::Settings::serialize() const
{
    json o = json::object();
    for (const auto &[k, v] : selected_symbols) {
        o[(std::string)k] = (std::string)v;
    }
    json j;
    j["selected_symbols"] = selected_symbols;
    return j;
}

void ToolHelperMapSymbol::Settings::load_from_json(const json &j)
{
    if (j.count("selected_symbols")) {
        for (const auto &[k, v] : j.at("selected_symbols").items()) {
            selected_symbols[UUID(k)] = UUID(v.get<std::string>());
        }
    }
}

} // namespace horizon
