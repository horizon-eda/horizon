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
        int index_a = doc.get_schematic()->sheets.at(a.sheet).index;
        int index_b = doc.get_schematic()->sheets.at(b.sheet).index;

        if (a.sheet == doc.get_sheet()->uuid)
            index_a = -1;
        if (b.sheet == doc.get_sheet()->uuid)
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
    for (const auto &it_sheet : doc.get_schematic()->sheets) {
        const auto &sheet = it_sheet.second;
        if (q.types.count(Type::SYMBOL_REFDES)) {
            for (const auto &it : sheet.symbols) {
                if (q.matches(it.second.component->refdes)) {
                    results.emplace_back(Type::SYMBOL_REFDES, it.first);
                    auto &x = results.back();
                    x.location = it.second.placement.shift;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::SYMBOL_PIN)) {
            for (const auto &it : sheet.symbols) {
                for (const auto &it_pin : it.second.symbol.pins) {
                    if (q.matches(it_pin.second.name)) {
                        results.emplace_back(Type::SYMBOL_PIN, it.first, it_pin.first);
                        auto &x = results.back();
                        x.location = it.second.placement.transform(it_pin.second.position);
                        x.sheet = sheet.uuid;
                        x.selectable = false;
                    }
                }
            }
        }
        if (q.types.count(Type::SYMBOL_MPN)) {
            for (const auto &it : sheet.symbols) {
                if (it.second.component->part && q.matches(it.second.component->part->get_MPN())) {
                    results.emplace_back(Type::SYMBOL_MPN, it.first);
                    auto &x = results.back();
                    x.location = it.second.placement.shift;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::NET_LABEL)) {
            for (const auto &it : sheet.net_labels) {
                if (it.second.junction->net && q.matches(it.second.junction->net->name)) {
                    results.emplace_back(Type::NET_LABEL, it.first);
                    auto &x = results.back();
                    x.location = it.second.junction->position;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::BUS_RIPPER)) {
            for (const auto &it : sheet.bus_rippers) {
                if (it.second.bus_member->net && q.matches(it.second.bus_member->net->name)) {
                    results.emplace_back(Type::BUS_RIPPER, it.first);
                    auto &x = results.back();
                    x.location = it.second.get_connector_pos();
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::POWER_SYMBOL)) {
            for (const auto &it : sheet.power_symbols) {
                if (it.second.junction->net && q.matches(it.second.junction->net->name)) {
                    results.emplace_back(Type::POWER_SYMBOL, it.first);
                    auto &x = results.back();
                    x.location = it.second.junction->position;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
        if (q.types.count(Type::TEXT)) {
            for (const auto &it : sheet.texts) {
                if (q.matches(it.second.text)) {
                    results.emplace_back(Type::TEXT, it.first);
                    auto &x = results.back();
                    x.location = it.second.placement.shift;
                    x.sheet = sheet.uuid;
                    x.selectable = true;
                }
            }
        }
    }
    sort_search_results_schematic(results, q);

    return results;
}
std::set<Searcher::Type> SearcherSchematic::get_types() const
{
    return {
            Type::TEXT,       Type::SYMBOL_REFDES, Type::SYMBOL_MPN, Type::NET_LABEL,
            Type::BUS_RIPPER, Type::POWER_SYMBOL,  Type::SYMBOL_PIN,
    };
}

std::string SearcherSchematic::get_display_name(const Searcher::SearchResult &r)
{
    if (r.type == Type::SYMBOL_PIN) {
        auto sym_name = doc.get_display_name(ObjectType::SCHEMATIC_SYMBOL, r.path.at(0), r.sheet);
        const auto &sym = doc.get_schematic()->sheets.at(r.sheet).symbols.at(r.path.at(0));
        return sym_name + "." + sym.gate->unit->pins.at(r.path.at(1)).primary_name;
    }
    else {
        return doc.get_display_name(get_type_info(r.type).object_type, r.path.at(0), r.sheet);
    }
}

} // namespace horizon
