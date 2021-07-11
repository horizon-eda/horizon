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

using WalkCB = std::function<void(const Sheet &, unsigned int, const Schematic &, const UUIDVec &)>;

struct WalkContext {
    WalkCB cb;
    const Schematic &top;
};

static void walk_sheets_rec(const Schematic &sch, const UUIDVec &instance_path, WalkContext &ctx)
{
    for (auto sheet : sch.get_sheets_sorted()) {
        const auto sheet_index = ctx.top.sheet_mapping.sheet_numbers.at(uuid_vec_append(instance_path, sheet->uuid));
        ctx.cb(*sheet, sheet_index, sch, instance_path);
        for (auto sym : sheet->get_block_symbols_sorted()) {
            walk_sheets_rec(*sym->schematic, uuid_vec_append(instance_path, sym->block_instance->uuid), ctx);
        }
    }
}

static void walk_sheets(const Schematic &sch, WalkCB cb)
{
    WalkContext ctx{cb, sch};
    walk_sheets_rec(sch, {}, ctx);
}

static const std::string &get_refdes(const Component &comp, const UUIDVec &instance_path, const Block &top_block)
{
    if (instance_path.size()) {
        return top_block.block_instance_mappings.at(instance_path).components.at(comp.uuid).refdes;
    }
    else {
        return comp.refdes;
    }
}

std::list<Searcher::SearchResult> SearcherSchematic::search(const Searcher::SearchQuery &q)
{
    std::list<SearchResult> results;
    if (!q.is_valid())
        return results;

    const auto &top = *doc.get_top_schematic();
    walk_sheets(top, [&results, &q, &top](const Sheet &sheet, unsigned int sheet_index, const Schematic &sch,
                                          const UUIDVec &instance_path) {
        if (q.types.count(Type::SYMBOL_REFDES)) {
            for (const auto &[uu, sym] : sheet.symbols) {
                if (q.matches(get_refdes(*sym.component, instance_path, *top.block))) {
                    results.emplace_back(Type::SYMBOL_REFDES, uu);
                    auto &x = results.back();
                    x.location = sym.placement.shift;
                    x.sheet = sheet.uuid;
                    x.instance_path = instance_path;
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
                        x.instance_path = instance_path;
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
                    x.instance_path = instance_path;
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
                    x.instance_path = instance_path;
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
                    x.instance_path = instance_path;
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
                    x.instance_path = instance_path;
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
                    x.instance_path = instance_path;
                    x.selectable = true;
                }
            }
        }
    });
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
    const auto &sch = doc.get_schematic_for_instance_path(r.instance_path);
    const auto &sheet = sch.sheets.at(r.sheet);
    const auto uu = r.path.at(0);
    switch (r.type) {
    case Type::SYMBOL_REFDES:
    case Type::SYMBOL_MPN: {
        const auto &sym = sheet.symbols.at(uu);
        return get_refdes(*sym.component, r.instance_path, *doc.get_top_block()) + sym.gate->suffix;
    }

    case Type::SYMBOL_PIN: {
        const auto &sym = sheet.symbols.at(uu);
        return get_refdes(*sym.component, r.instance_path, *doc.get_top_block()) + sym.gate->suffix
               + sym.gate->unit->pins.at(r.path.at(1)).primary_name;
    }

    case Type::TEXT:
        return sheet.texts.at(uu).text;

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
