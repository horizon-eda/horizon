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

    const auto &bl = *doc.get_current_block();
    {
        std::set<SelectableRef> new_sel;
        for (const auto &it : selection) {
            switch (it.type) {
            case ObjectType::SCHEMATIC_SYMBOL: {
                auto &sym = doc.get_sheet()->symbols.at(it.uuid);
                new_sel.emplace(sym.component->uuid, ObjectType::COMPONENT);
                for (const auto &it_txt : sym.texts) {
                    new_sel.emplace(it_txt->uuid, ObjectType::TEXT);
                }
            } break;

            case ObjectType::SCHEMATIC_BLOCK_SYMBOL: {
                auto &sym = doc.get_sheet()->block_symbols.at(it.uuid);
                new_sel.emplace(sym.block_instance->uuid, ObjectType::BLOCK_INSTANCE);
            } break;

            case ObjectType::LINE_NET: {
                auto &line = doc.get_sheet()->net_lines.at(it.uuid);
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
                auto &la = doc.get_sheet()->net_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::POWER_SYMBOL: {
                auto &ps = doc.get_sheet()->power_symbols.at(it.uuid);
                new_sel.emplace(ps.net->uuid, ObjectType::NET);
                new_sel.emplace(ps.junction->uuid, ObjectType::JUNCTION);
            } break;

            case ObjectType::BUS_LABEL: {
                const auto &la = doc.get_sheet()->bus_labels.at(it.uuid);
                new_sel.emplace(la.junction->uuid, ObjectType::JUNCTION);
                new_sel.emplace(la.bus->uuid, ObjectType::BUS);
            } break;

            case ObjectType::BUS_RIPPER: {
                const auto &rip = doc.get_sheet()->bus_rippers.at(it.uuid);
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

json ClipboardSchematic::serialize_junction(const class Junction &ju)
{
    if (auto sch_ju = dynamic_cast<const SchematicJunction *>(&ju)) {
        json j = sch_ju->serialize();
        if (sch_ju->net)
            j["net"] = (std::string)sch_ju->net->uuid;
        return j;
    }
    else {
        return ClipboardBase::serialize_junction(ju);
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
    j["block_instances"] = json::object();
    j["block_symbols"] = json::object();

    const auto &bl = *doc.get_current_block();

    std::set<UUID> nets;
    std::map<UUID, const LineNet *> net_lines;
    std::map<UUID, const SchematicSymbol *> symbols;
    std::map<UUID, const BusRipper *> bus_rippers;
    std::map<UUID, const SchematicBlockSymbol *> block_symbols;
    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::NET:
            nets.emplace(it.uuid);
            break;

        case ObjectType::LINE_NET:
            net_lines.emplace(it.uuid, &doc.get_sheet()->net_lines.at(it.uuid));
            break;

        case ObjectType::SCHEMATIC_SYMBOL:
            symbols.emplace(it.uuid, &doc.get_sheet()->symbols.at(it.uuid));
            break;

        case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
            block_symbols.emplace(it.uuid, &doc.get_sheet()->block_symbols.at(it.uuid));
            break;

        case ObjectType::BUS_RIPPER:
            bus_rippers.emplace(it.uuid, &doc.get_sheet()->bus_rippers.at(it.uuid));
            break;

        default:;
        }
    }


    for (const auto &it : selection) {
        switch (it.type) {
        case ObjectType::COMPONENT: {
            auto comp = bl.components.at(it.uuid);
            comp.refdes = comp.get_prefix() + "?";
            map_erase_if(comp.connections, [&nets](auto &x) { return nets.count(x.second.net.uuid) == 0; });
            map_erase_if(comp.connections, [&comp, &net_lines, &symbols](auto &x) {
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

        case ObjectType::BLOCK_INSTANCE: {
            auto inst = bl.block_instances.at(it.uuid);
            inst.refdes = "I?";
            map_erase_if(inst.connections, [&nets](auto &x) { return nets.count(x.second.net.uuid) == 0; });
            map_erase_if(inst.connections, [&inst, &net_lines, &block_symbols](auto &x) {
                for (const auto &[uu_line, line] : net_lines) {
                    if (line->net == x.second.net) {
                        for (auto &it_ft : {line->from, line->to}) {
                            if (it_ft.is_port()) {
                                for (const auto &[uu_sym, sym] : block_symbols) {
                                    if (sym->block_instance->uuid == inst.uuid && it_ft.block_symbol == sym) {
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
            j["block_instances"][(std::string)it.uuid] = inst.serialize();
        } break;

        case ObjectType::SCHEMATIC_SYMBOL:
            j["symbols"][(std::string)it.uuid] = doc.get_sheet()->symbols.at(it.uuid).serialize();
            break;

        case ObjectType::SCHEMATIC_BLOCK_SYMBOL:
            j["block_symbols"][(std::string)it.uuid] = doc.get_sheet()->block_symbols.at(it.uuid).serialize();
            break;

        case ObjectType::NET: {
            auto net = bl.nets.at(it.uuid);
            net.diffpair = nullptr;
            net.diffpair_master = false;
            j["nets"][(std::string)it.uuid] = net.serialize();
        } break;

        case ObjectType::LINE_NET: {
            auto line = doc.get_sheet()->net_lines.at(it.uuid);
            std::map<UUID, SchematicJunction> extra_junctions;
            for (auto &it_ft : {&line.from, &line.to}) {
                if (it_ft->is_bus_ripper()) {
                    if (bus_rippers.count(it_ft->bus_ripper->uuid) == 0) {
                        auto uu = UUID::random();
                        auto &ju = extra_junctions.emplace(uu, uu).first->second;
                        ju.net = line.net;
                        ju.position = it_ft->get_position();
                        it_ft->connect(&ju);
                    }
                }
                else if (it_ft->is_pin()) {
                    if (symbols.count(it_ft->symbol->uuid) == 0) {
                        auto uu = UUID::random();
                        auto &ju = extra_junctions.emplace(uu, uu).first->second;
                        ju.net = line.net;
                        ju.position = it_ft->get_position();
                        it_ft->connect(&ju);
                    }
                }
                else if (it_ft->is_port()) {
                    if (block_symbols.count(it_ft->block_symbol->uuid) == 0) {
                        auto uu = UUID::random();
                        auto &ju = extra_junctions.emplace(uu, uu).first->second;
                        ju.net = line.net;
                        ju.position = it_ft->get_position();
                        it_ft->connect(&ju);
                    }
                }
            }
            auto o = line.serialize();
            if (line.net)
                o["net"] = (std::string)line.net->uuid;
            j["net_lines"][(std::string)it.uuid] = o;
            for (const auto &[uu, ju] : extra_junctions) {
                j["junctions"][(std::string)uu] = serialize_junction(ju);
            }
        } break;

        case ObjectType::NET_LABEL:
            j["net_labels"][(std::string)it.uuid] = doc.get_sheet()->net_labels.at(it.uuid).serialize();
            break;

        case ObjectType::POWER_SYMBOL:
            j["power_symbols"][(std::string)it.uuid] = doc.get_sheet()->power_symbols.at(it.uuid).serialize();
            break;

        case ObjectType::BUS_LABEL:
            j["bus_labels"][(std::string)it.uuid] = doc.get_sheet()->bus_labels.at(it.uuid).serialize();
            break;

        case ObjectType::BUS_RIPPER:
            j["bus_rippers"][(std::string)it.uuid] = doc.get_sheet()->bus_rippers.at(it.uuid).serialize();
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
