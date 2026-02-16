#include "searcher_schematic.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/schematic.hpp"
#include "util/util.hpp"
#include "pool/part.hpp"

namespace horizon {
SearcherSchematic::SearcherSchematic(IDocumentSchematic &d) : doc(d)
{
}


void SearcherSchematic::sort_search_results_schematic(std::list<Searcher::SearchResult> &results,
                                                      const Searcher::SearchQuery &q)
{
    results.sort([this, q](const auto &a, const auto &b) {
        int index_a =
                doc.get_top_schematic()->sheet_mapping.sheet_numbers.at(uuid_vec_append(a.instance_path, a.sheet));
        int index_b =
                doc.get_top_schematic()->sheet_mapping.sheet_numbers.at(uuid_vec_append(b.instance_path, b.sheet));

        if (a.sheet == doc.get_sheet()->uuid && a.instance_path == doc.get_instance_path())
            index_a = -1;
        if (b.sheet == doc.get_sheet()->uuid && b.instance_path == doc.get_instance_path())
            index_b = -1;

        if (index_a > index_b)
            return false;
        if (index_a < index_b)
            return true;

        auto da = this->get_display_name(a);
        auto db = this->get_display_name(b);
        auto ina = !Coordf(a.location).in_range(q.area_visible.first, q.area_visible.second);
        auto inb = !Coordf(b.location).in_range(q.area_visible.first, q.area_visible.second);
        if (ina > inb)
            return false;
        else if (ina < inb)
            return true;

        if (a.type > b.type)
            return false;
        else if (a.type < b.type)
            return true;

        auto c = strcmp_natural(da, db);
        if (c > 0)
            return false;
        else if (c < 0)
            return true;

        if (a.location.x > b.location.x)
            return false;
        else if (a.location.x < b.location.x)
            return true;

        return a.location.y > b.location.y;
    });
}

std::list<Searcher::SearchResult> SearcherSchematic::search(const Searcher::SearchQuery &q)
{
    std::list<SearchResult> results;
    if (!q.is_valid())
        return results;

    if (doc.get_block_symbol_mode())
        return results;

    const auto &top = *doc.get_top_schematic();
    for (const auto &it : top.get_all_sheets()) {
        if (q.types.count(Type::SYMBOL_REFDES)) {
            for (const auto &[uu, sym] : it.sheet.symbols) {
                if (q.matches(top.block->get_component_info(*sym.component, it.instance_path).refdes)) {
                    results.emplace_back(Type::SYMBOL_REFDES, uu);
                    auto &x = results.back();
                    x.location = sym.placement.shift;
                    x.sheet = it.sheet.uuid;
                    x.instance_path = it.instance_path;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::SYMBOL_PIN)) {
            for (const auto &[uu_sym, sym] : it.sheet.symbols) {
                for (const auto &[uu_pin, pin] : sym.symbol.pins) {
                    if (q.matches(pin.name)) {
                        results.emplace_back(Type::SYMBOL_PIN, uu_sym, uu_pin);
                        auto &x = results.back();
                        x.location = sym.placement.transform(pin.position);
                        x.sheet = it.sheet.uuid;
                        x.instance_path = it.instance_path;
                        x.selectable = false;
                    }
                }
            }
        }
        if (q.types.count(Type::SYMBOL_MPN)) {
            for (const auto &[uu, sym] : it.sheet.symbols) {
                if (sym.component->part && q.matches(sym.component->part->get_MPN())) {
                    results.emplace_back(Type::SYMBOL_MPN, uu);
                    auto &x = results.back();
                    x.location = sym.placement.shift;
                    x.sheet = it.sheet.uuid;
                    x.instance_path = it.instance_path;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::NET_LABEL)) {
            for (const auto &[uu, label] : it.sheet.net_labels) {
                if (label.junction->net && q.matches(label.junction->net->name)) {
                    results.emplace_back(Type::NET_LABEL, uu);
                    auto &x = results.back();
                    x.location = label.junction->position;
                    x.sheet = it.sheet.uuid;
                    x.instance_path = it.instance_path;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::BUS_RIPPER)) {
            for (const auto &[uu, rip] : it.sheet.bus_rippers) {
                if (rip.bus_member->net && q.matches(rip.bus_member->net->name)) {
                    results.emplace_back(Type::BUS_RIPPER, uu);
                    auto &x = results.back();
                    x.location = rip.get_connector_pos();
                    x.sheet = it.sheet.uuid;
                    x.instance_path = it.instance_path;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::POWER_SYMBOL)) {
            for (const auto &[uu, sym] : it.sheet.power_symbols) {
                if (sym.junction->net && q.matches(sym.junction->net->name)) {
                    results.emplace_back(Type::POWER_SYMBOL, uu);
                    auto &x = results.back();
                    x.location = sym.junction->position;
                    x.sheet = it.sheet.uuid;
                    x.instance_path = it.instance_path;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::TEXT)) {
            for (const auto &[uu, text] : it.sheet.texts) {
                if (q.matches(text.text)) {
                    results.emplace_back(Type::TEXT, uu);
                    auto &x = results.back();
                    x.location = text.placement.shift;
                    x.sheet = it.sheet.uuid;
                    x.instance_path = it.instance_path;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::TABLE)) {
            for (const auto &[uu, table] : it.sheet.tables) {
                for (auto &cell : table.get_cells()) {
                    if (q.matches(cell)) {
                        results.emplace_back(Type::TABLE, uu);
                        auto &x = results.back();
                        x.location = table.placement.shift;
                        x.sheet = it.sheet.uuid;
                        x.instance_path = it.instance_path;
                        x.selectable = true;
                    }
                }
            }
        }
    };
    sort_search_results_schematic(results, q);

    return results;
}
std::set<Searcher::Type> SearcherSchematic::get_types() const
{
    return {
            Type::TEXT,       Type::SYMBOL_REFDES, Type::SYMBOL_MPN, Type::NET_LABEL,
            Type::BUS_RIPPER, Type::POWER_SYMBOL,  Type::SYMBOL_PIN, Type::TABLE,
    };
}

std::string SearcherSchematic::get_display_name(const Searcher::SearchResult &r)
{
    if (doc.get_block_symbol_mode())
        return "";

    const auto &sch = doc.get_schematic_for_instance_path(r.instance_path);
    const auto &sheet = sch.sheets.at(r.sheet);
    const auto uu = r.path.at(0);
    switch (r.type) {
    case Type::SYMBOL_REFDES:
    case Type::SYMBOL_MPN: {
        const auto &sym = sheet.symbols.at(uu);
        return doc.get_top_block()->get_component_info(*sym.component, r.instance_path).refdes + sym.gate->suffix;
    }

    case Type::SYMBOL_PIN: {
        const auto &sym = sheet.symbols.at(uu);
        return doc.get_top_block()->get_component_info(*sym.component, r.instance_path).refdes + sym.gate->suffix
               + sym.gate->unit->pins.at(r.path.at(1)).primary_name;
    }

    case Type::TEXT:
        return sheet.texts.at(uu).text;

    case Type::TABLE:
        // FIXME: find more meaningful display text
        return "Table";

    case Type::NET_LABEL: {
        auto &ju = *sheet.net_labels.at(uu).junction;
        return ju.net ? ju.net->name : "";
    }

    case Type::BUS_RIPPER:
        return sheet.bus_rippers.at(uu).bus_member->net->name;

    case Type::POWER_SYMBOL:
        return sheet.power_symbols.at(uu).net->name;

    default:
        return "???";
    }
}

} // namespace horizon
