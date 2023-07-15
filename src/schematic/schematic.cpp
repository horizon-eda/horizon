#include "schematic.hpp"
#include "common/lut.hpp"
#include "pool/entity.hpp"
#include <set>
#include <forward_list>
#include <iostream>
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "logger/logger.hpp"
#include "util/picture_load.hpp"
#include "pool/ipool.hpp"
#include "util/str_util.hpp"
#include "iinstancce_mapping_provider.hpp"

namespace horizon {

static const LutEnumStr<Schematic::Annotation::Order> annotation_order_lut = {
        {"down_right", Schematic::Annotation::Order::DOWN_RIGHT},
        {"right_down", Schematic::Annotation::Order::RIGHT_DOWN},
};
static const LutEnumStr<Schematic::Annotation::Mode> annotation_mode_lut = {
        {"sequential", Schematic::Annotation::Mode::SEQUENTIAL},
        {"sheet_100", Schematic::Annotation::Mode::SHEET_100},
        {"sheet_1000", Schematic::Annotation::Mode::SHEET_1000},
};

Schematic::Annotation::Annotation()
{
}

Schematic::Annotation::Annotation(const json &j)
{
    if (!j.is_null()) {
        order = annotation_order_lut.lookup(j.at("order"));
        mode = annotation_mode_lut.lookup(j.at("mode"));
        fill_gaps = j.at("fill_gaps").get<bool>();
        keep = j.at("keep").get<bool>();
        ignore_unknown = j.value("ignore_unknown", false);
    }
}

json Schematic::Annotation::serialize() const
{
    json j;
    j["order"] = annotation_order_lut.lookup_reverse(order);
    j["mode"] = annotation_mode_lut.lookup_reverse(mode);
    j["fill_gaps"] = fill_gaps;
    j["keep"] = keep;
    j["ignore_unknown"] = ignore_unknown;
    return j;
}

static const unsigned int app_version = 8;

unsigned int Schematic::get_app_version()
{
    return app_version;
}

Schematic::Schematic(const UUID &uu, const json &j, Block &iblock, IPool &pool, IBlockSymbolAndSchematicProvider &prv)
    : uuid(uu), block(&iblock), name(j.at("name").get<std::string>()),
      group_tag_visible(j.value("group_tag_visible", false)), annotation(j.value("annotation", json())),
      version(app_version, j)
{
    {
        const json &o = j["sheets"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            sheets.emplace(std::make_pair(u, Sheet(u, it.value(), *block, pool, prv)));
        }
    }
    if (j.count("rules")) {
        rules.load_from_json(j.at("rules"));
    }
    if (j.count("title_block_values")) {
        const json &o = j["title_block_values"];
        for (const auto &[key, value] : o.items()) {
            block->project_meta[key] = value.get<std::string>();
        }
    }
    if (j.count("pdf_export_settings")) {
        try {
            pdf_export_settings = PDFExportSettings(j.at("pdf_export_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load pdf export settings", Logger::Domain::SCHEMATIC, e.what());
        }
    }

    update_refs();
}

Schematic Schematic::new_from_file(const std::string &filename, Block &block, IPool &pool,
                                   IBlockSymbolAndSchematicProvider &prv)
{
    auto j = load_json_from_file(filename);
    return Schematic(UUID(j.at("uuid").get<std::string>()), j, block, pool, prv);
}

Schematic::Schematic(const UUID &uu, Block &bl) : uuid(uu), block(&bl), version(app_version)
{
    auto suu = UUID::random();
    sheets.emplace(suu, suu);
}


void Schematic::autoconnect_symbol(Sheet *sheet, SchematicSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->symbols.at(sym->uuid));
    bool connected = false;
    for (auto &it_pin : sym->symbol.pins) {
        auto pin_pos = sym->placement.transform(it_pin.second.position);
        if (sym->component->connections.count(UUIDPath<2>(sym->gate->uuid, it_pin.first)))
            continue; // pin is connected, don't connect
        for (auto it_junc = sheet->junctions.begin(); it_junc != sheet->junctions.end();) {
            bool erase = false;
            if (!it_junc->second.bus && it_junc->second.position == pin_pos
                && (it_junc->second.net || it_junc->second.connected_net_lines.size()
                    || it_junc->second.connected_power_symbols.size())) {
                erase = true;
                bool net_created = false;
                if (it_junc->second.net == nullptr) {
                    it_junc->second.net = block->insert_net();
                    net_created = true;
                }
                sym->component->connections.emplace(UUIDPath<2>(sym->gate->uuid, it_pin.first),
                                                    static_cast<Net *>(it_junc->second.net));
                bool has_power_sym = false;
                for (const auto &it_ps : sheet->power_symbols) {
                    if (it_ps.second.junction->uuid == it_junc->first) {
                        has_power_sym = true;
                        break;
                    }
                }
                if (has_power_sym) {
                    erase = false;
                    // create line
                    auto uu = UUID::random();
                    auto *line = &sheet->net_lines.emplace(uu, uu).first->second;
                    line->net = it_junc->second.net;
                    line->from.connect(sym, &it_pin.second);
                    line->to.connect(&it_junc->second);
                }
                else {
                    sheet->replace_junction(&it_junc->second, sym, &it_pin.second);
                }
                connected = true;

                if (net_created)
                    expand(true);
            }
            if (erase) {
                sheet->junctions.erase(it_junc++);
                break;
            }
            else {
                it_junc++;
            }
        }
        for (auto &it_sym_other : sheet->symbols) {
            auto *sym_other = &it_sym_other.second;
            if (sym_other == sym)
                continue;
            for (auto &it_pin_other : sym_other->symbol.pins) {
                auto pin_pos_other = sym_other->placement.transform(it_pin_other.second.position);
                if (sym_other->component->connections.count(UUIDPath<2>(sym_other->gate->uuid, it_pin_other.first)))
                    continue; // other pin is connected, don't connect

                if (pin_pos == pin_pos_other) {
                    // create net
                    auto uu = UUID::random();
                    auto *net = block->insert_net();

                    // connect pin
                    sym->component->connections.emplace(UUIDPath<2>(sym->gate->uuid, it_pin.first), net);
                    // connect other pin
                    sym_other->component->connections.emplace(UUIDPath<2>(sym_other->gate->uuid, it_pin_other.first),
                                                              net);

                    uu = UUID::random();
                    auto *line = &sheet->net_lines.emplace(uu, uu).first->second;
                    line->net = net;
                    line->from.connect(sym, &it_pin.second);
                    line->to.connect(sym_other, &it_pin_other.second);
                    break;
                    connected = true;
                }
            }
        }
        for (auto &it_rip : sheet->bus_rippers) {
            if (pin_pos == it_rip.second.get_connector_pos()) {
                sym->component->connections.emplace(UUIDPath<2>(sym->gate->uuid, it_pin.first),
                                                    static_cast<Net *>(it_rip.second.bus_member->net));
                auto uu = UUID::random();
                auto *line = &sheet->net_lines.emplace(uu, uu).first->second;
                line->net = it_rip.second.bus_member->net;
                line->from.connect(sym, &it_pin.second);
                line->to.connect(&it_rip.second);
                connected = true;
                break;
            }
        }
    }
    if (connected)
        expand(true);
}


void Schematic::autoconnect_block_symbol(Sheet *sheet, SchematicBlockSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->block_symbols.at(sym->uuid));
    bool connected = false;
    for (auto &[port_uuz, sym_port] : sym->symbol.ports) {
        auto port_pos = sym->placement.transform(sym_port.position);
        if (sym->block_instance->connections.count(sym_port.net))
            continue; // pin is connected, don't connect
        for (auto it_junc = sheet->junctions.begin(); it_junc != sheet->junctions.end();) {
            bool erase = false;
            if (!it_junc->second.bus && it_junc->second.position == port_pos
                && (it_junc->second.net || it_junc->second.connected_net_lines.size()
                    || it_junc->second.connected_power_symbols.size())) {
                erase = true;
                bool net_created = false;
                if (it_junc->second.net == nullptr) {
                    it_junc->second.net = block->insert_net();
                    net_created = true;
                }
                sym->block_instance->connections.emplace(sym_port.net, static_cast<Net *>(it_junc->second.net));
                bool has_power_sym = false;
                for (const auto &it_ps : sheet->power_symbols) {
                    if (it_ps.second.junction->uuid == it_junc->first) {
                        has_power_sym = true;
                        break;
                    }
                }
                if (has_power_sym) {
                    erase = false;
                    // create line
                    auto uu = UUID::random();
                    auto *line = &sheet->net_lines.emplace(uu, uu).first->second;
                    line->net = it_junc->second.net;
                    line->from.connect(sym, &sym_port);
                    line->to.connect(&it_junc->second);
                }
                else {
                    sheet->replace_junction(&it_junc->second, sym, &sym_port);
                }
                connected = true;

                if (net_created)
                    expand(true);
            }
            if (erase) {
                sheet->junctions.erase(it_junc++);
                break;
            }
            else {
                it_junc++;
            }
        }
    }
    if (connected)
        expand(true);
}

void Schematic::disconnect_symbol(Sheet *sheet, SchematicSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->symbols.at(sym->uuid));
    std::map<const SymbolPin *, SchematicJunction *> pin_junctions;
    for (auto &it_line : sheet->net_lines) {
        LineNet *line = &it_line.second;
        // if((line->to_symbol && line->to_symbol->uuid == it.uuid) ||
        // (line->from_symbol &&line->from_symbol->uuid == it.uuid)) {
        for (auto it_ft : {&line->to, &line->from}) {
            if (it_ft->symbol == sym) {
                SchematicJunction *j = nullptr;
                if (pin_junctions.count(it_ft->pin)) {
                    j = pin_junctions.at(it_ft->pin);
                }
                else {
                    auto uu = UUID::random();
                    auto x = pin_junctions.emplace(it_ft->pin, &sheet->junctions.emplace(uu, uu).first->second);
                    j = x.first->second;
                }
                auto c = it_ft->get_position();
                j->position = c;
                it_ft->connect(j);
            }
        }
        for (auto it_conn = sym->component->connections.cbegin(); it_conn != sym->component->connections.cend();) {
            if (it_conn->first.at(0) == sym->gate->uuid) {
                sym->component->connections.erase(it_conn++);
            }
            else {
                it_conn++;
            }
        }
    }
}

void Schematic::disconnect_block_symbol(Sheet *sheet, SchematicBlockSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->block_symbols.at(sym->uuid));
    std::map<const BlockSymbolPort *, SchematicJunction *> port_junctions;
    for (auto &[uu_line, line] : sheet->net_lines) {
        for (auto it_ft : {&line.to, &line.from}) {
            if (it_ft->block_symbol == sym) {
                SchematicJunction *j = nullptr;
                if (port_junctions.count(it_ft->port)) {
                    j = port_junctions.at(it_ft->port);
                }
                else {
                    auto uu = UUID::random();
                    j = port_junctions.emplace(it_ft->port, &sheet->junctions.emplace(uu, uu).first->second)
                                .first->second;
                }
                auto c = it_ft->get_position();
                j->position = c;
                it_ft->connect(j);
            }
        }
        sym->block_instance->connections.clear();
    }
}


void Schematic::smash_symbol(Sheet *sheet, SchematicSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->symbols.at(sym->uuid));
    if (sym->smashed)
        return;
    sym->smashed = true;
    for (const auto &it : sym->pool_symbol->texts) {
        auto uu = UUID::random();
        auto &x = sheet->texts.emplace(uu, uu).first->second;
        x.from_smash = true;
        x.placement = sym->placement;
        Placement placement = it.second.placement;
        if (sym->symbol.texts.count(it.first)) {
            placement = sym->symbol.texts.at(it.first).placement;
        }
        x.placement.accumulate(placement);
        x.text = it.second.text;
        x.layer = it.second.layer;
        x.size = it.second.size;
        x.width = it.second.width;
        sym->texts.push_back(&x);
    }
}

static void replace_substr(std::string &str, const std::string &needle, const std::string &rep)
{
    auto pos = str.find(needle);
    if (pos != std::string::npos) {
        str.replace(pos, needle.size(), rep);
    }
}

void Schematic::unsmash_symbol(Sheet *sheet, SchematicSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->symbols.at(sym->uuid));
    if (!sym->smashed)
        return;
    sym->smashed = false;
    for (auto &it : sym->texts) {
        if (it->from_smash) {
            sheet->texts.erase(it->uuid); // expand will delete from sym->texts
        }
    }
}

bool Schematic::delete_net_line(Sheet *sheet, LineNet *line)
{
    bool split = false;
    if (line->net) {
        for (auto &it_ft : {line->from, line->to}) {
            if (it_ft.is_pin()) {
                UUIDPath<2> conn_path(it_ft.symbol->gate.uuid, it_ft.pin->uuid);
                if (it_ft.symbol->component->connections.count(conn_path) && it_ft.pin->connection_count <= 1) {
                    it_ft.symbol->component->connections.erase(conn_path);
                }
            }
            else if (it_ft.is_port()) {
                const auto net_port = it_ft.port->net;
                if (it_ft.block_symbol->block_instance->connections.count(net_port)
                    && it_ft.port->connection_count <= 1) {
                    it_ft.block_symbol->block_instance->connections.erase(net_port);
                }
            }
        }
    }
    auto from = line->from;
    auto to = line->to;
    Net *net = line->net;
    sheet->net_lines.erase(line->uuid);
    line = nullptr;
    sheet->propagate_net_segments();
    if (net) {
        UUID from_net_segment = from.get_net_segment();
        UUID to_net_segment = to.get_net_segment();
        if (from_net_segment != to_net_segment) {
            auto pins_from = sheet->get_pins_connected_to_net_segment(from_net_segment);
            auto pins_to = sheet->get_pins_connected_to_net_segment(to_net_segment);
            std::cout << "!!!net split" << std::endl;
            if (!net->is_named() && !net->is_power && !net->is_bussed) {
                // net is unnamed, not bussed and not a power net, user
                // does't care which pins get extracted
                // imp->tool_bar_flash("net split");
                block->extract_pins(pins_to);
            }
            else if (net->is_power) {
                auto ns_info = sheet->analyze_net_segments();
                if (ns_info.count(from_net_segment) && ns_info.count(to_net_segment)) {
                    auto &inf_from = ns_info.at(from_net_segment);
                    auto &inf_to = ns_info.at(to_net_segment);
                    if (inf_from.has_power_sym && inf_to.has_power_sym) {
                        // both have label, don't need to split net
                    }
                    else if (inf_from.has_power_sym && !inf_to.has_power_sym) {
                        // from has label, to not, extracts pins on to
                        // net segment
                        // imp->tool_bar_flash("net split");
                        block->extract_pins(pins_to);
                    }
                    else if (!inf_from.has_power_sym && inf_to.has_power_sym) {
                        // to has label, from not, extract pins on from
                        // segment
                        // imp->tool_bar_flash("net split");
                        block->extract_pins(pins_from);
                    }
                    else {
                        // imp->tool_bar_flash("net split");
                        block->extract_pins(pins_from);
                    }
                }
            }
            else {
                auto ns_info = sheet->analyze_net_segments();
                if (ns_info.count(from_net_segment) && ns_info.count(to_net_segment)) {
                    auto &inf_from = ns_info.at(from_net_segment);
                    auto &inf_to = ns_info.at(to_net_segment);
                    if (inf_from.has_label && inf_to.has_label) {
                        // both have label, don't need to split net
                    }
                    else if (inf_from.has_label && !inf_to.has_label) {
                        // from has label, to not, extracts pins on to
                        // net segment
                        // imp->tool_bar_flash("net split");
                        block->extract_pins(pins_to);
                    }
                    else if (!inf_from.has_label && inf_to.has_label) {
                        // to has label, from not, extract pins on from
                        // segment
                        // imp->tool_bar_flash("net split");
                        block->extract_pins(pins_from);
                    }
                    else if (!inf_from.has_label && !inf_to.has_label) {
                        // both segments are unlabeled, so don't care
                        // imp->tool_bar_flash("net split");
                        block->extract_pins(pins_from);
                    }
                }
            }
        }
    }
    return split;
}

bool Schematic::place_bipole_on_line(Sheet *sheet, SchematicSymbol *sym)
{
    bool placed = false;
    if (sym->symbol.pins.size() == 2) {
        auto it_sym = sym->symbol.pins.begin();
        auto &pin1 = it_sym->second;
        it_sym++;
        auto &pin2 = it_sym->second;
        if ((pin1.position.x == pin2.position.x) || (pin1.position.y == pin2.position.y)) {
            std::cout << "place bipole" << std::endl;
            auto pin1_pos = sym->placement.transform(pin1.position);
            auto pin2_pos = sym->placement.transform(pin2.position);
            LineNet *line = nullptr;
            for (auto &[li_uu, li] : sheet->net_lines) {
                if ((li.coord_on_line(pin1_pos) || li.from.get_position() == pin1_pos
                     || li.to.get_position() == pin1_pos)
                    && (li.coord_on_line(pin2_pos) || li.from.get_position() == pin2_pos
                        || li.to.get_position() == pin2_pos)) {
                    line = &li;
                    break;
                }
            }
            if (line) {
                auto from = line->from;
                auto to = line->to;
                const auto dst_normal =
                        (pin1_pos - from.get_position()).mag_sq() + (pin2_pos - to.get_position()).mag_sq();
                const auto dst_swapped =
                        (pin2_pos - from.get_position()).mag_sq() + (pin1_pos - to.get_position()).mag_sq();
                SymbolPin *pin_from = &pin1;
                SymbolPin *pin_to = &pin2;
                if (dst_swapped < dst_normal) {
                    std::swap(pin_from, pin_to);
                }
                auto connect_pin = [sheet, sym, line](SymbolPin &pin, const LineNet::Connection &conn) {
                    auto uu = UUID::random();
                    auto &new_line = sheet->net_lines.emplace(uu, uu).first->second;
                    new_line.from = conn;
                    if (conn.is_pin())
                        conn.pin->connection_count++;
                    new_line.to.connect(sym, &pin);
                    new_line.net = line->net;
                    sym->component->connections.emplace(std::piecewise_construct,
                                                        std::forward_as_tuple(sym->gate->uuid, pin.uuid),
                                                        std::forward_as_tuple(line->net));
                };
                connect_pin(*pin_from, from);
                connect_pin(*pin_to, to);
                delete_net_line(sheet, line);
                line = nullptr;
                placed = true;
                expand(true);
            }
        }
    }
    return placed;
}

void Schematic::expand_connectivity(bool careful)
{
    for (auto &it_net : block->nets) {
        it_net.second.is_power_forced = false;
        it_net.second.is_bussed = false;
        it_net.second.keep = false;
    }
    for (auto &it_bus : block->buses) {
        for (auto &it_mem : it_bus.second.members) {
            it_mem.second.net->is_bussed = true;
        }
    }

    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        for (auto &it_sym : sheet.power_symbols) {
            it_sym.second.net->is_power = true;
            it_sym.second.net->is_power_forced = true;
        }
    }

    block->update_diffpairs();

    block->update_connection_count(); // also sets has_bus_rippers to false
    for (auto &it : block->buses) {
        it.second.is_referenced = false;
    }

    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        for (auto &it_rip : sheet.bus_rippers) {
            it_rip.second.bus_member->net->has_bus_rippers = true;
            it_rip.second.bus->is_referenced = true;
        }
        for (auto &it_la : sheet.bus_labels) {
            it_la.second.bus->is_referenced = true;
        }
    }

    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        sheet.warnings.clear();

        for (auto &it_junc : sheet.junctions) {
            it_junc.second.net = nullptr;
            it_junc.second.bus = nullptr;
        }
        for (auto &it_line : sheet.net_lines) {
            it_line.second.net = nullptr;
            it_line.second.bus = nullptr;
        }

        sheet.delete_dependants();
        sheet.update_junction_connections();
        sheet.propagate_net_segments();
        if (!careful)
            sheet.vacuum_junctions();
        sheet.update_junction_connections();

        auto net_segments = sheet.analyze_net_segments();


        for (auto &it_sym : sheet.power_symbols) {
            if (it_sym.second.junction->net_segment) {
                // net segments of single power symbols aren't included in
                // net_segments
                if (net_segments.count(it_sym.second.junction->net_segment)) {
                    auto &ns = net_segments.at(it_sym.second.junction->net_segment);
                    if (ns.net == nullptr) {
                        ns.net = it_sym.second.net;
                    }
                    if (ns.net != it_sym.second.net) {
                        throw std::runtime_error("illegal power connection");
                    }
                }
            }
        }
        for (auto &it_rip : sheet.bus_rippers) {
            if (it_rip.second.net_segment) {
                if (net_segments.count(it_rip.second.net_segment)) {
                    auto &ns = net_segments.at(it_rip.second.net_segment);
                    if (ns.net == nullptr) {
                        ns.net = it_rip.second.bus_member->net;
                    }
                    if (ns.net != it_rip.second.bus_member->net) {
                        throw std::runtime_error("illegal bus ripper connection");
                    }
                }
            }
        }
        for (auto &it_la : sheet.bus_labels) {
            if (it_la.second.junction->net_segment) {
                // net segments of single power symbols aren't included in
                // net_segments
                if (net_segments.count(it_la.second.junction->net_segment)) {
                    auto &ns = net_segments.at(it_la.second.junction->net_segment);
                    if (ns.net != nullptr) {
                        throw std::runtime_error("bus/net conflict");
                    }
                    if (ns.bus == nullptr) {
                        ns.bus = it_la.second.bus;
                    }
                    if (ns.bus != it_la.second.bus) {
                        throw std::runtime_error("illegal bus connection");
                    }
                }
            }
        }

        for (auto &it_sym : sheet.symbols) {
            SchematicSymbol &schsym = it_sym.second;
            Component *comp = schsym.component;
            for (const auto &it_conn : comp->connections) {
                const Gate &gate_conn = comp->entity->gates.at(it_conn.first.at(0));
                const Pin &pin = gate_conn.unit->pins.at(it_conn.first.at(1));
                if ((gate_conn.uuid == schsym.gate->uuid)) { // connection with net
                    if (schsym.symbol.pins.count(pin.uuid)) {
                        SymbolPin &sympin = schsym.symbol.pins.at(pin.uuid);
                        if (it_conn.second.net) {
                            sympin.connector_style = SymbolPin::ConnectorStyle::NONE;
                            if (sympin.net_segment) {
                                auto &ns = net_segments.at(sympin.net_segment);
                                if (ns.is_bus()) {
                                    throw std::runtime_error("illegal bus-pin connection");
                                }
                                if (ns.net == nullptr) {
                                    ns.net = it_conn.second.net;
                                }
                                if (ns.net != it_conn.second.net) {
                                    throw std::runtime_error("illegal connection");
                                }
                            }
                        }
                        else {
                            sympin.connector_style = SymbolPin::ConnectorStyle::NC;
                        }
                    }
                    else {
                        Logger::log_critical("Pin " + gate_conn.name + "." + pin.primary_name + " not found on symbol "
                                                     + schsym.component->refdes + schsym.gate->suffix
                                                     + " but is connected to net "
                                                     + block->get_net_name(it_conn.second.net->uuid),
                                             Logger::Domain::SCHEMATIC);
                    }
                }
            }
        }
        for (auto &[uu_sym, schsym] : sheet.block_symbols) {
            const auto &inst = *schsym.block_instance;
            for (const auto &[net_port, conn] : inst.connections) {
                if (auto symport = schsym.symbol.get_port_for_net(net_port)) {
                    if (conn.net) {
                        symport->connector_style = BlockSymbolPort::ConnectorStyle::NONE;
                        if (symport->net_segment) {
                            auto &ns = net_segments.at(symport->net_segment);
                            if (ns.is_bus()) {
                                throw std::runtime_error("illegal bus-pin connection");
                            }
                            if (ns.net == nullptr) {
                                ns.net = conn.net;
                            }
                            if (ns.net != conn.net) {
                                throw std::runtime_error("illegal connection");
                            }
                        }
                    }
                    else {
                        symport->connector_style = BlockSymbolPort::ConnectorStyle::NC;
                    }
                }
                else {
                    if (conn.net)
                        Logger::log_critical("Port " + inst.block->nets.at(net_port).name
                                                     + " not found on block symbol " + schsym.block_instance->refdes
                                                     + " but is connected to net "
                                                     + block->get_net_name(conn.net->uuid),
                                             Logger::Domain::SCHEMATIC);
                    else
                        Logger::log_critical("Port " + inst.block->nets.at(net_port).name
                                                     + " not found on block symbol " + schsym.block_instance->refdes
                                                     + " but is set to no connect",
                                             Logger::Domain::SCHEMATIC);
                }
            }
        }
        for (auto &[uu_label, label] : sheet.net_labels) {
            if (block->nets.count(label.last_net) && net_segments.count(label.junction->net_segment)) {
                auto &ns = net_segments.at(label.junction->net_segment);
                if (ns.net == nullptr)
                    ns.net = &block->nets.at(label.last_net);
            }
        }
        for (auto &it_line : sheet.net_lines) {
            it_line.second.net = net_segments.at(it_line.second.net_segment).net;
            it_line.second.bus = net_segments.at(it_line.second.net_segment).bus;
        }
        for (auto &it_junc : sheet.junctions) {
            if (net_segments.count(it_junc.second.net_segment)) {
                it_junc.second.net = net_segments.at(it_junc.second.net_segment).net;
                it_junc.second.bus = net_segments.at(it_junc.second.net_segment).bus;
            }
        }
        for (auto &[uu_label, label] : sheet.net_labels) {
            if (label.junction->net)
                label.last_net = label.junction->net->uuid;
            else
                label.last_net = UUID();

            if (block->nets.count(label.last_net))
                block->nets.at(label.last_net).keep = true;
        }
        sheet.update_junction_connections();
    }

    if (!careful) {
        block->vacuum_nets();
        block->vacuum_group_tag_names();
    }


    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        if (!careful) {
            sheet.fix_junctions();
            sheet.update_junction_connections();
            sheet.delete_duplicate_net_lines();
            sheet.update_junction_connections();
            sheet.simplify_net_lines();
            sheet.update_junction_connections();
        }
        sheet.propagate_net_segments();
        const auto nsinfo = sheet.analyze_net_segments();
        sheet.place_warnings(nsinfo);
    }
    update_refs();

    for (auto &[uu, sheet] : sheets) {
        sheet.update_bus_ripper_connections();
    }

    // warn juncs
    for (auto &it_sheet : sheets) {
        std::set<Coordi> pin_coords;
        std::set<Coordi> junction_coords;
        Sheet &sheet = it_sheet.second;
        for (auto &it_junc : sheet.junctions) {
            Junction &junc = it_junc.second;
            junction_coords.insert(junc.position);
        }
        for (auto &it_sym : sheet.symbols) {
            SchematicSymbol &schsym = it_sym.second;
            for (auto &it_pin : schsym.symbol.pins) {
                auto pin_pos = schsym.placement.transform(it_pin.second.position);
                auto r = pin_coords.insert(pin_pos);
                if (!r.second)
                    sheet.warnings.emplace_back(pin_pos, "Pin on pin");
                if (junction_coords.count(pin_pos))
                    sheet.warnings.emplace_back(pin_pos, "Pin on junction");
                if (it_pin.second.connection_count == 0) {
                    const auto path = UUIDPath<2>(schsym.gate->uuid, it_pin.second.uuid);
                    if (schsym.component->connections.count(path)) {
                        auto &c = schsym.component->connections.at(path);
                        if (c.net)
                            sheet.warnings.emplace_back(pin_pos,
                                                        "Pin connected to net: " + block->get_net_name(c.net->uuid));
                    }
                }

                for (auto &it_line : sheet.net_lines) {
                    LineNet &line = it_line.second;
                    if (line.coord_on_line(pin_pos)) {
                        if (!line.is_connected_to_symbol(it_sym.first, it_pin.first)) {
                            sheet.warnings.emplace_back(pin_pos, "Pin on Line");
                        }
                    }
                }
            }
        }
        for (const auto &[uu_sym, sym] : sheet.block_symbols) {
            for (const auto &[uu_port, port] : sym.symbol.ports) {
                if (port.net) {
                    auto pin_pos = sym.placement.transform(port.position);
                    if (port.connection_count == 0) {
                        if (sym.block_instance->connections.count(port.net)) {
                            auto &c = sym.block_instance->connections.at(port.net);
                            if (c.net)
                                sheet.warnings.emplace_back(pin_pos, "Port connected to net: "
                                                                             + block->get_net_name(c.net->uuid));
                        }
                    }
                }
            }
        }
    }

    // warn bus rippers
    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        for (auto &it_rip : sheet.bus_rippers) {
            BusRipper &ripper = it_rip.second;
            if (ripper.junction->bus != ripper.bus) {
                sheet.warnings.emplace_back(ripper.junction->position, "Bus ripper connected to wrong net line");
            }
        }
    }

    std::map<UUID, std::set<unsigned int>> nets_on_sheets;
    for (auto &it : sheets) {
        auto nsinfo = it.second.analyze_net_segments();
        for (auto &it_ns : nsinfo) {
            if (it_ns.second.net) {
                auto net_uuid = it_ns.second.net->uuid;
                if (nets_on_sheets.count(net_uuid) == 0) {
                    nets_on_sheets.emplace(net_uuid, std::set<unsigned int>());
                }
                nets_on_sheets.at(net_uuid).insert(it.second.index);
            }
        }
    }
    for (auto &it : sheets) {
        for (auto &it_label : it.second.net_labels) {
            if (it_label.second.junction->net) {
                if (nets_on_sheets.count(it_label.second.junction->net->uuid)) {
                    it_label.second.on_sheets = nets_on_sheets.at(it_label.second.junction->net->uuid);
                    it_label.second.on_sheets.erase(it.second.index);
                }
            }
        }
    }
}

void Schematic::expand(bool careful, const IInstanceMappingProvider *inst_map_prv)
{
    const BlockInstanceMapping *inst_map = nullptr;
    if (inst_map_prv) {
        if ((inst_map = inst_map_prv->get_block_instance_mapping()))
            assert(inst_map->block == block->uuid);
    }

    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        for (auto &it : sheet.texts) {
            it.second.overridden = false;
        }
    }

    // collect gates
    if (!careful) {
        std::set<UUIDPath<2>> gates;
        for (const auto &it_component : block->components) {
            for (const auto &it_gate : it_component.second.entity->gates) {
                gates.emplace(it_component.first, it_gate.first);
            }
        }

        for (auto &it_sheet : sheets) {
            Sheet &sheet = it_sheet.second;
            std::vector<UUID> keys;
            keys.reserve(sheet.symbols.size());
            for (const auto &it : sheet.symbols) {
                keys.push_back(it.first);
            }
            for (const auto &uu : keys) {
                const auto &sym = sheet.symbols.at(uu);
                if (gates.count({sym.component->uuid, sym.gate->uuid})) {
                    // all fine
                    gates.erase({sym.component->uuid, sym.gate->uuid});
                }
                else { // duplicate symbol
                    sheet.symbols.erase(uu);
                }
            }
        }
    }

    for (auto &[uu, comp] : block->components) {
        using N = Component::NopopulateFromInstance;
        if (inst_map) {
            if (inst_map->components.count(uu)) {
                if (inst_map->components.at(uu).nopopulate)
                    comp.nopopulate_from_instance = N::SET;
                else
                    comp.nopopulate_from_instance = N::CLEAR;
            }
            else {
                comp.nopopulate_from_instance = N::CLEAR;
            }
        }
        else {
            comp.nopopulate_from_instance = N::UNSET;
        }
    }


    for (auto &[uu_sheet, sheet] : sheets) {
        // disconnected deleted ports
        for (auto &[uu_sym, sym] : sheet.block_symbols) {
            auto &prv_sym = *sym.prv_symbol;
            map_erase_if(sym.block_instance->connections,
                         [&prv_sym](const auto &conn) { return prv_sym.get_port_for_net(conn.first) == nullptr; });
        }

        // delete net lines connected to deleted ports
        map_erase_if(sheet.net_lines, [](auto &x) {
            for (auto it_ft : {&x.second.from, &x.second.to}) {
                if (it_ft->is_port()) {
                    if (it_ft->block_symbol->prv_symbol->ports.count(it_ft->port.uuid) == 0) { // port is gone
                        return true;
                    }
                }
            }
            return false;
        });
    }

    {
        std::set<UUID> insts;
        for (const auto &[uu_inst, inst] : block->block_instances) {
            for (const auto &[uu_port, conn] : inst.connections) {
                auto &port_net = inst.block->nets.at(uu_port);
                if (!port_net.is_port) {
                    Logger::log_critical("block instance " + inst.refdes + " has connection at non-port net "
                                                 + inst.block->get_net_name(uu_port),
                                         Logger::Domain::BLOCK);
                }
            }
            insts.insert(uu_inst);
        }
        for (const auto &[uu_sheet, sheet] : sheets) {
            for (const auto &[uu_sym, sym] : sheet.block_symbols) {
                insts.erase(sym.block_instance->uuid);
            }
        }
        for (const auto &uu : insts) {
            Logger::log_critical("block instance " + block->block_instances.at(uu).refdes + " (" + (std::string)uu
                                         + ") has no block symbol",
                                 Logger::Domain::BLOCK);
        }
    }

    for (auto &[uu_sheet, sheet] : sheets) {
        for (auto &[uu_sym, sym] : sheet.symbols) {
            auto &texts = sym.texts;
            auto &sh = sheet;
            texts.erase(std::remove_if(texts.begin(), texts.end(),
                                       [&sh](const auto &a) { return sh.texts.count(a.uuid) == 0; }),
                        texts.end());
        }
        if (!careful)
            sheet.expand_symbols(*this, inst_map);
    }

    expand_connectivity(careful);


    std::set<const Net *> nets_witout_port;
    for (const auto &[uu, net] : block->nets) {
        if (net.is_port)
            nets_witout_port.insert(&net);
    }

    for (auto &[uu_sheet, sheet] : sheets) {
        for (const auto &[uu, la] : sheet.net_labels) {
            if (la.show_port)
                nets_witout_port.erase(la.junction->net);
        }
    }


    std::map<std::string, std::set<const Net *>> net_names;
    for (const auto &[uu, net] : block->nets) {
        if (net.is_named()) {
            std::string net_name = net.name;
            trim(net_name);
            const std::string net_name_casefold = Glib::ustring(net_name).casefold();
            net_names[net_name_casefold].insert(&net);
        }
    }
    std::set<const Net *> nets_duplicate;
    for (const auto &it : net_names) {
        if (it.second.size() > 1) { // duplicate net name
            nets_duplicate.insert(it.second.begin(), it.second.end());
        }
    }
    for (auto &it_sheet : sheets) {
        for (const auto &it : it_sheet.second.analyze_net_segments()) {
            if (nets_duplicate.count(it.second.net)) {
                it_sheet.second.warnings.emplace_back(it.second.position, "Duplicate net name");
            }
            if (nets_witout_port.count(it.second.net)) {
                it_sheet.second.warnings.emplace_back(it.second.position, "Need 'show port' net label");
            }
        }
    }

    {
        std::set<std::string> all_refdes;
        std::set<std::string> duplicate_refdes;
        for (const auto &it_comp : block->components) {
            const auto &refdes = it_comp.second.refdes;
            if (refdes.size() && refdes.back() != '?') {
                if (all_refdes.count(refdes)) {
                    duplicate_refdes.insert(refdes);
                }
                else {
                    all_refdes.insert(refdes);
                }
            }
        }

        for (auto &it_sheet : sheets) {
            for (auto &it_sym : it_sheet.second.symbols) {
                const auto &refdes = it_sym.second.component->refdes;
                if (duplicate_refdes.count(refdes))
                    it_sheet.second.warnings.emplace_back(it_sym.second.placement.shift, "Duplicate refdes " + refdes);
            }
        }
    }

    {
        for (auto &[sheet_uu, sheet] : sheets) {
            for (const auto &[sym_uu, sym] : sheet.block_symbols) {
                std::map<UUID, const Net *> ports;
                for (const auto &[net_uu, net] : sym.block_instance->block->nets) {
                    if (net.is_port)
                        ports.emplace(net_uu, &net);
                }
                for (const auto &[port_uu, port] : sym.symbol.ports) {
                    ports.erase(port.net);
                }
                if (ports.size()) {
                    std::string txt = "Missing ports:";
                    for (const auto &[net_uu, net] : ports) {
                        txt += " " + net->name;
                    }
                    sheet.warnings.emplace_back(sym.placement.shift, txt);
                }
            }
        }
    }

    expand_frames(inst_map_prv);

    if (!careful) {
        for (auto &it_sheet : sheets) {
            Sheet &sheet = it_sheet.second;
            for (auto &it_text : sheet.texts) {
                auto &text = it_text.second;
                if (!text.overridden) {
                    Glib::ustring txt = text.text;
                    Glib::MatchInfo ma;
                    int start, end;
                    while (get_sheetref_regex()->match(txt, ma) && ma.fetch_pos(0, start, end)) {
                        auto ref_uuid = UUID(ma.fetch(1));
                        if (sheets.count(ref_uuid)) {
                            std::stringstream ss;
                            ss << "[" << sheets.at(ref_uuid).index << "]";
                            txt.replace(start, end - start, ss.str());
                        }
                        else {
                            txt.replace(start, end - start, "[?]");
                        }
                        text.overridden = true;
                    }
                    text.text_override = txt;
                }
            }
        }
    }
}

void Schematic::expand_frames(const class IInstanceMappingProvider *inst_map)
{
    const unsigned int sheet_total = inst_map ? inst_map->get_sheet_total() : sheets.size();

    for (auto &[uu, sheet] : sheets) {
        if (sheet.pool_frame) {
            const unsigned int sheet_idx = inst_map ? inst_map->get_sheet_index(uu) : sheet.index;

            std::map<std::string, std::string> values = block->project_meta;
            for (const auto &it_v : sheet.title_block_values) {
                values[it_v.first] = it_v.second;
            }
            sheet.frame = *sheet.pool_frame;
            for (auto &it_text : sheet.frame.texts) {
                auto &txt = it_text.second.text;

                replace_substr(txt, "$sheet_idx", std::to_string(sheet_idx));
                replace_substr(txt, "$sheet_total", std::to_string(sheet_total));
                replace_substr(txt, "$sheet_title", sheet.name);
                for (const auto &it_v : values) {
                    replace_substr(txt, "$" + it_v.first, it_v.second);
                }
            }
        }
        else {
            sheet.frame = Frame(UUID());
        }
    }
}

struct AnnotationContext {
    Schematic &top;
    std::map<std::string, std::vector<unsigned int>> refdes;
};

static void visit_schematic_for_annotation(Schematic &sch, const UUIDVec &instance_path, AnnotationContext &ctx)
{
    auto &annotation = ctx.top.annotation;
    using Annotation = Schematic::Annotation;
    if (!ctx.top.annotation.keep) {
        for (auto &[uu, comp] : sch.block->components) {
            ctx.top.block->set_refdes(comp, instance_path, "?");
        }
    }

    for (const auto &[uu, comp] : sch.block->components) {
        ctx.refdes[comp.get_prefix()];
    }

    for (auto sheet : sch.get_sheets_sorted()) {
        const auto sheet_index = ctx.top.sheet_mapping.sheet_numbers.at(uuid_vec_append(instance_path, sheet->uuid));
        unsigned int sheet_offset = 0;
        unsigned int sheet_incr = 0;
        if (annotation.mode != Annotation::Mode::SEQUENTIAL) {
            for (auto &it : ctx.refdes) {
                it.second.clear();
            }
            if (annotation.mode == Annotation::Mode::SHEET_100) {
                sheet_offset = 100 * sheet_index;
                sheet_incr = 100;
            }
            else if (annotation.mode == Annotation::Mode::SHEET_1000) {
                sheet_offset = 1000 * sheet_index;
                sheet_incr = 1000;
            }
        }

        std::vector<SchematicSymbol *> symbols;
        symbols.reserve(sheet->symbols.size());
        std::transform(sheet->symbols.begin(), sheet->symbols.end(), std::back_inserter(symbols),
                       [](auto &a) { return &a.second; });
        switch (annotation.order) {
        case Annotation::Order::RIGHT_DOWN:
            std::sort(symbols.begin(), symbols.end(),
                      [](const auto a, const auto b) { return a->placement.shift.x < b->placement.shift.x; });
            std::stable_sort(symbols.begin(), symbols.end(),
                             [](const auto a, const auto b) { return a->placement.shift.y > b->placement.shift.y; });
            break;
        case Annotation::Order::DOWN_RIGHT:
            std::sort(symbols.begin(), symbols.end(),
                      [](const auto a, const auto b) { return a->placement.shift.y > b->placement.shift.y; });
            std::stable_sort(symbols.begin(), symbols.end(),
                             [](const auto a, const auto b) { return a->placement.shift.x < b->placement.shift.x; });
            break;
        }

        for (const auto sym : symbols) {
            auto &v = ctx.refdes[sym->component->get_prefix()];
            const auto rd = ctx.top.block->get_refdes(*sym->component, instance_path);
            if (rd.find('?') == std::string::npos) { // already annotated
                auto ss = rd.substr(sym->component->get_prefix().size());
                int si = -1;
                try {
                    si = std::stoi(ss);
                }
                catch (const std::invalid_argument &e) {
                    if (!annotation.ignore_unknown)
                        ctx.top.block->set_refdes(*sym->component, instance_path, "?");
                }
                if (si > 0) {
                    v.push_back(si);
                    std::sort(v.begin(), v.end());
                }
            }
        }

        for (const auto sym : symbols) {
            auto &v = ctx.refdes[sym->component->get_prefix()];
            const auto rd = ctx.top.block->get_refdes(*sym->component, instance_path);
            if (rd.find('?') != std::string::npos) { // needs annotatation
                unsigned int n = 1 + sheet_offset;
                if (v.size() != 0) {
                    if (annotation.fill_gaps && v.size() >= 2) {
                        bool hole = false;
                        for (auto it = v.begin(); it < v.end() - 1; it++) {
                            if (*it > sheet_offset && *(it + 1) != (*it) + 1) {
                                n = (*it) + 1;
                                hole = true;
                                break;
                            }
                        }
                        if (!hole) {
                            n = v.back() + 1;
                        }
                    }
                    else {
                        n = v.back() + 1;
                    }
                }
                if (sheet_incr && n / sheet_incr != sheet_index) {
                    n = sheet_offset + 1;
                }
                v.push_back(n);
                ctx.top.block->set_refdes(*sym->component, instance_path,
                                          sym->component->get_prefix() + std::to_string(n));
            }
            std::sort(v.begin(), v.end());
        }

        for (auto sym : sheet->get_block_symbols_sorted()) {
            visit_schematic_for_annotation(*sym->schematic, uuid_vec_append(instance_path, sym->block_instance->uuid),
                                           ctx);
        }
    }
}

void Schematic::annotate()
{
    AnnotationContext ctx{*this, {}};
    visit_schematic_for_annotation(*this, {}, ctx);
}

std::map<UUIDPath<2>, std::string> Schematic::get_unplaced_gates() const
{
    std::map<UUIDPath<2>, std::string> unplaced;
    // collect gates
    std::set<std::pair<const Component *, const Gate *>> gates;

    // find all gates
    for (const auto &it_component : block->components) {
        for (const auto &it_gate : it_component.second.entity->gates) {
            gates.emplace(&it_component.second, &it_gate.second);
        }
    }

    // remove placed gates
    for (auto &it_sheet : sheets) {
        const auto &sheet = it_sheet.second;

        for (const auto &it_sym : sheet.symbols) {
            const auto &sym = it_sym.second;
            gates.erase({sym.component, sym.gate});
        }
    }
    for (const auto &it : gates) {
        unplaced.emplace(std::piecewise_construct, std::forward_as_tuple(it.first->uuid, it.second->uuid),
                         std::forward_as_tuple(it.first->refdes + it.second->suffix));
    }
    return unplaced;
}

void Schematic::swap_gates(const UUID &comp_uu, const UUID &g1_uu, const UUID &g2_uu)
{
    block->swap_gates(comp_uu, g1_uu, g2_uu);
    auto entity = block->components.at(comp_uu).entity;
    for (auto &it_sheet : sheets) {
        for (auto &it_sym : it_sheet.second.symbols) {
            if (it_sym.second.component->uuid == comp_uu) {
                if (it_sym.second.gate->uuid == g1_uu) {
                    it_sym.second.gate = &entity->gates.at(g2_uu);
                }
                else if (it_sym.second.gate->uuid == g2_uu) {
                    it_sym.second.gate = &entity->gates.at(g1_uu);
                }
            }
        }
    }
}

Glib::RefPtr<Glib::Regex> Schematic::get_sheetref_regex()
{
    static auto regex =
            Glib::Regex::create(R"(\$sheetref:([a-f0-9]{8}-[a-f0-9]{4}-4[a-f0-9]{3}-[89aAbB][a-f0-9]{3}-[a-f0-9]{12}))",
                                Glib::RegexCompileFlags::REGEX_CASELESS | Glib::RegexCompileFlags::REGEX_OPTIMIZE);
    return regex;
}

Schematic::Schematic(const Schematic &sch)
    : uuid(sch.uuid), block(sch.block), name(sch.name), sheets(sch.sheets), rules(sch.rules),
      group_tag_visible(sch.group_tag_visible), annotation(sch.annotation), sheet_mapping(sch.sheet_mapping),
      pdf_export_settings(sch.pdf_export_settings), version(sch.version)
{
    update_refs();
}

void Schematic::update_refs()
{
    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        for (auto &it_sym : sheet.symbols) {
            SchematicSymbol &schsym = it_sym.second;
            schsym.component.update(block->components);
            schsym.gate.update(schsym.component->entity->gates);
            for (auto &it_text : schsym.texts) {
                it_text.update(sheet.texts);
            }
        }
        for (auto &[uu, sym] : sheet.block_symbols) {
            sym.block_instance.update(block->block_instances);
        }
        for (auto &it_line : sheet.net_lines) {
            LineNet &line = it_line.second;
            line.update_refs(sheet);
            line.net.update(block->nets);
            line.bus.update(block->buses);
        }
        for (auto &it_line : sheet.lines) {
            Line &line = it_line.second;
            line.from.update(sheet.junctions);
            line.to.update(sheet.junctions);
        }
        for (auto &it_arc : sheet.arcs) {
            Arc &arc = it_arc.second;
            arc.from.update(sheet.junctions);
            arc.to.update(sheet.junctions);
            arc.center.update(sheet.junctions);
        }
        for (auto &it_junc : sheet.junctions) {
            it_junc.second.net.update(block->nets);
            it_junc.second.bus.update(block->buses);
        }
        for (auto &it_sym : sheet.power_symbols) {
            it_sym.second.update_refs(sheet, *block);
        }
        for (auto &it_label : sheet.net_labels) {
            it_label.second.junction.update(sheet.junctions);
        }
        for (auto &it_bus_label : sheet.bus_labels) {
            it_bus_label.second.junction.update(sheet.junctions);
            it_bus_label.second.bus.update(block->buses);
        }
        for (auto &it_sym : sheet.bus_rippers) {
            it_sym.second.update_refs(sheet, *block);
        }
        for (auto &[uu, tie] : sheet.net_ties) {
            tie.net_tie.update(block->net_ties);
            tie.update_refs(sheet);
        }
    }
}

json Schematic::serialize() const
{
    json j;
    version.serialize(j);
    j["type"] = "schematic_block";
    j["uuid"] = (std::string)uuid;
    j["block"] = (std::string)block->uuid;
    j["name"] = name;
    j["annotation"] = annotation.serialize();
    j["pdf_export_settings"] = pdf_export_settings.serialize_schematic();
    j["rules"] = rules.serialize();
    j["group_tag_visible"] = group_tag_visible;
    j["title_block_values"] = block->project_meta;

    j["sheets"] = json::object();
    for (const auto &it : sheets) {
        j["sheets"][(std::string)it.first] = it.second.serialize();
    }

    return j;
}

void Schematic::load_pictures(const std::string &dir)
{
    std::list<std::map<UUID, Picture> *> pictures;
    for (auto &it : sheets) {
        pictures.push_back(&it.second.pictures);
    }
    pictures_load(pictures, dir, "sch");
}

ItemSet Schematic::get_pool_items_used() const
{
    ItemSet items_needed;

    for (const auto &it_sheet : sheets) {
        for (const auto &it_sym : it_sheet.second.symbols) {
            items_needed.emplace(ObjectType::SYMBOL, it_sym.second.pool_symbol->uuid);
        }
        if (it_sheet.second.pool_frame)
            items_needed.emplace(ObjectType::FRAME, it_sheet.second.pool_frame->uuid);
    }
    return items_needed;
}

template <typename T, typename U> static std::vector<T *> sort_sheets(U &sch)
{
    std::vector<T *> sheets;
    for (auto &[uu, it] : sch.sheets) {
        sheets.push_back(&it);
    }
    std::sort(sheets.begin(), sheets.end(), [](auto a, auto b) { return a->index < b->index; });
    return sheets;
}

std::vector<Sheet *> Schematic::get_sheets_sorted()
{
    return sort_sheets<Sheet>(*this);
}

std::vector<const Sheet *> Schematic::get_sheets_sorted() const
{
    return sort_sheets<const Sheet>(*this);
}


void Schematic::SheetMapping::update(const Schematic &sch, const UUIDVec &instance_path)
{
    if (Block::instance_path_too_long(instance_path, __FUNCTION__))
        return;
    for (const auto sh : sch.get_sheets_sorted()) {
        sheet_numbers.emplace(uuid_vec_append(instance_path, sh->uuid), index);
        index++;
        for (const auto sym : sh->get_block_symbols_sorted()) {
            update(*sym->schematic, uuid_vec_append(instance_path, sym->block_instance->uuid));
        }
    }
}

void Schematic::SheetMapping::update(const Schematic &sch)
{
    index = 1;
    sheet_numbers.clear();
    update(sch, {});
    sheet_total = index - 1;
}

void Schematic::update_sheet_mapping()
{
    sheet_mapping.update(*this);
}


template <bool c>
using WalkCB =
        std::function<void(make_const_ref_t<c, Sheet>, unsigned int, make_const_ref_t<c, Schematic>, const UUIDVec &)>;

template <bool c> struct WalkContextSchematic {
    WalkCB<c> cb;
    const Schematic &top;
};

template <bool c>
static void walk_sheets_rec(make_const_ref_t<c, Schematic> sch, const UUIDVec &instance_path,
                            WalkContextSchematic<c> &ctx)
{
    if (Block::instance_path_too_long(instance_path, __FUNCTION__))
        return;
    for (auto sheet : sch.get_sheets_sorted()) {
        const auto sheet_index = ctx.top.sheet_mapping.sheet_numbers.at(uuid_vec_append(instance_path, sheet->uuid));
        ctx.cb(*sheet, sheet_index, sch, instance_path);
        for (auto sym : sheet->get_block_symbols_sorted()) {
            walk_sheets_rec(*sym->schematic, uuid_vec_append(instance_path, sym->block_instance->uuid), ctx);
        }
    }
}

template <bool c> static void walk_sheets(make_const_ref_t<c, Schematic> sch, WalkCB<c> cb)
{
    WalkContextSchematic<c> ctx{cb, sch};
    walk_sheets_rec(sch, {}, ctx);
}

std::vector<Schematic::SheetItem<true>> Schematic::get_all_sheets() const
{
    std::vector<Schematic::SheetItem<true>> items;
    walk_sheets<true>(*this, [&items](const Sheet &sheet, unsigned int sheet_index, const Schematic &sch,
                                      const UUIDVec &instance_path) {
        items.emplace_back(sheet, sheet_index, sch, instance_path);
    });
    return items;
}

std::vector<Schematic::SheetItem<false>> Schematic::get_all_sheets()
{
    std::vector<Schematic::SheetItem<false>> items;
    walk_sheets<false>(*this,
                       [&items](Sheet &sheet, unsigned int sheet_index, Schematic &sch, const UUIDVec &instance_path) {
                           items.emplace_back(sheet, sheet_index, sch, instance_path);
                       });
    return items;
}

Sheet &Schematic::add_sheet()
{
    auto uu = UUID::random();
    auto sheet_max = std::max_element(sheets.begin(), sheets.end(),
                                      [](const auto &p1, const auto &p2) { return p1.second.index < p2.second.index; });
    auto &sheet = sheets.emplace(uu, uu).first->second;
    sheet.index = sheet_max->second.index + 1;
    sheet.name = "sheet " + std::to_string(sheet.index);
    return sheet;
}

void Schematic::delete_sheet(const UUID &uu)
{
    if (sheets.size() <= 1)
        return;
    if (!sheets.at(uu).can_be_removed()) // only delete empty sheets
        return;
    auto deleted_index = sheets.at(uu).index;
    sheets.erase(uu);
    for (auto &it : sheets) {
        if (it.second.index > deleted_index) {
            it.second.index--;
        }
    }
}

const Sheet &Schematic::get_sheet_at_index(unsigned int index) const
{
    auto x = std::find_if(sheets.begin(), sheets.end(), [index](auto e) { return e.second.index == index; });
    if (x == sheets.end())
        throw std::runtime_error("sheet " + std::to_string(index) + "not found");
    else
        return x->second;
}


Sheet &Schematic::get_sheet_at_index(unsigned int index)
{
    auto x = std::find_if(sheets.begin(), sheets.end(), [index](auto e) { return e.second.index == index; });
    if (x == sheets.end())
        throw std::runtime_error("sheet " + std::to_string(index) + "not found");
    else
        return x->second;
}

} // namespace horizon
