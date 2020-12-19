#include "clipboard_schematic.hpp"
#include "document/idocument_schematic.hpp"
#include "schematic/sheet.hpp"
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "pool/entity.hpp"

namespace horizon {

void ClipboardSchematic::expand_selection()
{
    ClipboardBase::expand_selection();

    const auto &sh = *doc.get_sheet();
    const auto &bl = *doc.get_block();
    {
        std::set<SelectableRef> new_sel;
        for (const auto &it : selection) {
            switch (it.type) {
            case ObjectType::SCHEMATIC_SYMBOL: {
                auto &sym = sh.symbols.at(it.uuid);
                new_sel.emplace(sym.component->uuid, ObjectType::COMPONENT);
                for (const auto &it_txt : sym.texts) {
                    new_sel.emplace(it_txt->uuid, ObjectType::TEXT);
                }
            } break;

            case ObjectType::LINE_NET: {
                auto &line = sh.net_lines.at(it.uuid);
                for (auto &it_ft : {line.from, line.to}) {
                    if (it_ft.is_junc()) {
                        new_sel.emplace(it_ft.junc.uuid, ObjectType::JUNCTION);
                    }
                }
                if (line.net)
                    new_sel.emplace(line.net->uuid, ObjectType::NET);
                else if (line.bus)
                    new_sel.emplace(line.bus->uuid, ObjectType::BUS);
            } break;

            case ObjectType::NET_LABEL: {
                auto &la = sh.net_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::POWER_SYMBOL: {
                auto &ps = sh.power_symbols.at(it.uuid);
                new_sel.emplace(ps.net->uuid, ObjectType::NET);
                new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::BUS_LABEL: {
                const auto &la = sh.bus_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
                new_sel.emplace(la.bus->uuid, ObjectType::BUS);
            } break;

            case ObjectType::BUS_RIPPER: {
                const auto &rip = sh.bus_rippers.at(it.uuid);
                new_sel.emplace(rip.junction->uuid, ObjectType::JUNCTION);
                new_sel.emplace(rip.bus->uuid, ObjectType::BUS);
            } break;

            default:;
            }
        }
        selection.insert(new_sel.begin(), new_sel.end());
    }

    {
        std::set<SelectableRef> new_sel;
        for (const auto &it : selection) {
            if (it.type == ObjectType::BUS) {
                const auto &bus = bl.buses.at(it.uuid);
                for (const auto &it_m : bus.members) {
                    new_sel.emplace(it_m.second.net->uuid, ObjectType::NET);
                }
            }
        }
        selection.insert(new_sel.begin(), new_sel.end());
    }
}

void ClipboardSchematic::serialize(json &j)
{
    ClipboardBase::serialize(j);

    j["components"] = json::object();
    j["symbols"] = json::object();
    j["nets"] = json::object();
    j["net_lines"] = json::object();
    j["net_labels"] = json::object();
    j["power_symbols"] = json::object();
    j["bus_labels"] = json::object();
    j["bus_rippers"] = json::object();

    const auto &sh = *doc.get_sheet();
    const auto &bl = *doc.get_block();

    std::set<UUID> nets;
    std::map<UUID, const LineNet *> net_lines;
    std::map<UUID, const SchematicSymbol *> symbols;
    std::map<UUID, const BusRipper *> bus_rippers;
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::NET:
            nets.emplace(it.uuid);
            break;

        case ObjectType::LINE_NET:
            net_lines.emplace(it.uuid, &sh.net_lines.at(it.uuid));
            break;

        case ObjectType::SCHEMATIC_SYMBOL:
            symbols.emplace(it.uuid, &sh.symbols.at(it.uuid));
            break;

        case ObjectType::BUS_RIPPER:
            bus_rippers.emplace(it.uuid, &sh.bus_rippers.at(it.uuid));
            break;

        default:;
        }
    }


    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::COMPONENT: {
            auto comp = bl.components.at(it.uuid);
            comp.refdes = comp.entity->prefix + "?";
            map_erase_if(comp.connections, [&nets](auto &x) { return nets.count(x.second.net.uuid) == 0; });
            map_erase_if(comp.connections, [this, &comp, &net_lines, &symbols](auto &x) {
                for (const auto &[uu_line, line] : net_lines) {
                    if (line->net == x.second.net) {
                        for (auto &it_ft : {line->from, line->to}) {
                            if (it_ft.is_pin()) {
                                for (const auto &[uu_sym, sym] : symbols) {
                                    if (sym->component->uuid == comp.uuid && sym->gate->uuid == x.first.at(0)
                                        && it_ft.symbol == sym) {
                                        return false;
                                    }
                                }
                            }
                        }
                        // return true;
                    }
                }
                return true;
            });
            j["components"][(std::string)it.uuid] = comp.serialize();
        } break;

        case ObjectType::SCHEMATIC_SYMBOL:
            j["symbols"][(std::string)it.uuid] = sh.symbols.at(it.uuid).serialize();
            break;

        case ObjectType::NET: {
            auto net = bl.nets.at(it.uuid);
            net.diffpair = nullptr;
            net.diffpair_master = false;
            j["nets"][(std::string)it.uuid] = net.serialize();
        } break;

        case ObjectType::LINE_NET: {
            auto line = sh.net_lines.at(it.uuid);
            std::map<UUID, SchematicJunction> extra_junctions;
            for (auto &it_ft : {&line.from, &line.to}) {
                if (it_ft->is_bus_ripper()) {
                    if (bus_rippers.count(it_ft->bus_ripper->uuid) == 0) {
                        auto uu = UUID::random();
                        auto &ju = extra_junctions.emplace(uu, uu).first->second;
                        ju.position = it_ft->get_position();
                        it_ft->connect(&ju);
                    }
                }
                else if (it_ft->is_pin()) {
                    if (symbols.count(it_ft->symbol->uuid) == 0) {
                        auto uu = UUID::random();
                        auto &ju = extra_junctions.emplace(uu, uu).first->second;
                        ju.position = it_ft->get_position();
                        it_ft->connect(&ju);
                    }
                }
            }
            j["net_lines"][(std::string)it.uuid] = line.serialize();
            for (const auto &[uu, ju] : extra_junctions) {
                j["junctions"][(std::string)uu] = ju.serialize();
            }
        } break;

        case ObjectType::NET_LABEL:
            j["net_labels"][(std::string)it.uuid] = sh.net_labels.at(it.uuid).serialize();
            break;

        case ObjectType::POWER_SYMBOL:
            j["power_symbols"][(std::string)it.uuid] = sh.power_symbols.at(it.uuid).serialize();
            break;

        case ObjectType::BUS_LABEL:
            j["bus_labels"][(std::string)it.uuid] = sh.bus_labels.at(it.uuid).serialize();
            break;

        case ObjectType::BUS_RIPPER:
            j["bus_rippers"][(std::string)it.uuid] = sh.bus_rippers.at(it.uuid).serialize();
            break;

        default:;
        }
    }
}

IDocument &ClipboardSchematic::get_doc()
{
    return doc;
}
} // namespace horizon
