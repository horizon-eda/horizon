#include "schematic.hpp"
#include "util/util.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"
#include "blocks/blocks_schematic.hpp"


namespace horizon {
RulesCheckResult SchematicRules::check_connectivity(const BlocksSchematic &blocks, class RulesCheckCache &cache) const
{
    RulesCheckResult r;
    if (r.check_disabled(rule_connectivity))
        return r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &rule = rule_connectivity;
    auto &c = cache.get_cache<RulesCheckCacheNetPins>();
    const auto &top = blocks.get_top_block_item().block;

    for (const auto &[net_uu, it] : c.get_net_pins()) {
        if (rule.include_unnamed || it.name.size()) {
            if (it.pins.size() == 1 && !it.is_nc) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &x = r.errors.back();
                auto &conn = it.pins.front();
                std::string refdes;
                if (conn.instance_path.size()) {
                    auto &comps = top.block_instance_mappings.at(conn.instance_path).components;
                    if (comps.count(conn.comp)) {
                        refdes = comps.at(conn.comp).refdes;
                    }
                    else {
                        const Block *block = &top;
                        for (const auto &uu : conn.instance_path)
                            block = block->block_instances.at(uu).block;
                        refdes = block->components.at(conn.comp).refdes;
                    }
                }
                else {
                    refdes = top.components.at(conn.comp).refdes;
                }
                x.comment = "Net \"" + it.name + "\" only connected to " + refdes + conn.gate.suffix + "."
                            + conn.pin.primary_name;
                x.sheet = conn.sheet;
                x.instance_path = conn.instance_path;
                x.location = conn.location;
                x.has_location = true;
            }
        }
    }
    const auto all_sheets = blocks.get_top_block_item().schematic.get_all_sheets();


    for (const auto &[uu_block, block_item] : blocks.blocks) {
        std::map<UUID, std::set<UUID>> net_segments_with_labels;
        for (const auto &[uu_sheet, sheet] : block_item.schematic.sheets) {
            auto net_segments = sheet.analyze_net_segments();
            for (const auto &[ns, nsinfo] : net_segments) {
                if (nsinfo.really_has_label()) {
                    if (!nsinfo.net->is_port)
                        net_segments_with_labels[nsinfo.net->uuid].insert(ns);
                }
            }
        }
        for (const auto &[net, net_segments] : net_segments_with_labels) {
            if (net_segments.size() == 1) {
                const auto seg = *net_segments.begin();
                r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                auto &x = r.errors.back();
                x.comment = "Net \"" + block_item.block.get_net_name(net) + "\" only has one net segment with a label";
                // find sheet the only label is on
                bool found = false;
                for (const auto &[uu_sheet, sheet] : block_item.schematic.sheets) {
                    for (const auto &[uu_label, label] : sheet.net_labels) {
                        if (label.junction->net->uuid == net && label.junction->net_segment == seg) {
                            // find first sheet to place warning on
                            for (const auto &it : all_sheets) {
                                if (it.schematic.block->uuid == uu_block && it.sheet.uuid == uu_sheet) {
                                    x.has_location = true;
                                    if (it.instance_path.size())
                                        x.comment += " (Only shown in first instance)";
                                    x.instance_path = it.instance_path;
                                    x.sheet = it.sheet.uuid;
                                    x.location = label.junction->position;
                                    break;
                                }
                            }

                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                }
            }
        }
    }

    for (const auto &[uu_block, block_item] : blocks.blocks) {
        for (const auto &[uu_comp, comp] : block_item.block.components) {
            std::set<UUIDPath<2>> pins;
            for (const auto &[uu_gate, gate] : comp.entity->gates) {
                for (const auto &[uu_pin, pin] : gate.unit->pins) {
                    pins.emplace(uu_gate, uu_pin);
                }
            }
            for (const auto &[path, net] : comp.connections)
                pins.erase(path);

            for (const auto &pin_path : pins) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &x = r.errors.back();
                const auto &gate = comp.entity->gates.at(pin_path.at(0));
                x.comment = "Pin " + comp.refdes + gate.suffix + "." + gate.unit->pins.at(pin_path.at(1)).primary_name
                            + " is not connected (not found in schematic)";
                bool found = false;
                for (const auto &it : all_sheets) {
                    for (const auto &[uu_sym, sym] : it.sheet.symbols) {
                        if (sym.component->uuid == uu_comp && sym.gate->uuid == pin_path.at(0)) {
                            x.comment = "Pin "
                                        + blocks.get_top_block_item()
                                                  .block.get_component_info(comp, it.instance_path)
                                                  .refdes
                                        + sym.gate->suffix + "." + sym.gate->unit->pins.at(pin_path.at(1)).primary_name
                                        + " is not connected";
                            x.has_location = true;
                            if (it.instance_path.size())
                                x.comment += " (Only shown in first instance)";
                            x.instance_path = it.instance_path;
                            x.sheet = it.sheet.uuid;
                            x.location = sym.placement.transform(sym.symbol.pins.at(pin_path.at(1)).position);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                }
            }
        }
    }

    for (const auto &[uu_block, block_item] : blocks.blocks) {
        for (const auto &[uu_inst, inst] : block_item.block.block_instances) {
            std::set<UUID> ports;
            for (const auto &[uu_net, net] : inst.block->nets) {
                if (net.is_port)
                    ports.insert(uu_net);
            }
            for (const auto &[uu_port, conn] : inst.connections)
                ports.erase(uu_port);

            for (const auto &uu_port : ports) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &x = r.errors.back();
                x.comment = "Port " + inst.refdes + "." + inst.block->nets.at(uu_port).name + " is not connected";
                bool found = false;
                for (const auto &it : all_sheets) {
                    for (const auto &[uu_sym, sym] : it.sheet.block_symbols) {
                        if (sym.block_instance->uuid == uu_inst) {
                            x.has_location = true;
                            if (it.instance_path.size())
                                x.comment += " (Only shown in first instance)";
                            x.instance_path = it.instance_path;
                            x.sheet = it.sheet.uuid;
                            x.location = sym.placement.transform(
                                    sym.symbol.ports.at(BlockSymbolPort::get_uuid_for_net(uu_port)).position);
                            found = true;
                            break;
                        }
                    }
                    if (found)
                        break;
                }
                if (!found)
                    x.comment += " (Not found on schematic)";
            }
        }
    }

    r.update();
    return r;
}

RulesCheckResult SchematicRules::check(RuleID id, const BlocksSchematic &blocks, RulesCheckCache &cache) const
{
    switch (id) {
    case RuleID::CONNECTIVITY:
        return check_connectivity(blocks, cache);

    default:
        return RulesCheckResult();
    }
}
} // namespace horizon
