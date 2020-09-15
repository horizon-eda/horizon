#include "schematic.hpp"
#include "common/lut.hpp"
#include "pool/entity.hpp"
#include <set>
#include <forward_list>
#include "nlohmann/json.hpp"
#include "util/util.hpp"
#include "logger/logger.hpp"
#include "util/picture_load.hpp"
#include "pool/ipool.hpp"

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
        fill_gaps = j.at("fill_gaps");
        keep = j.at("keep");
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

Schematic::Schematic(const UUID &uu, const json &j, Block &iblock, IPool &pool)
    : uuid(uu), block(&iblock), name(j.at("name").get<std::string>()),
      group_tag_visible(j.value("group_tag_visible", false)), annotation(j.value("annotation", json()))
{
    {
        const json &o = j["sheets"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            sheets.emplace(std::make_pair(u, Sheet(u, it.value(), *block, pool)));
        }
    }
    if (j.count("rules")) {
        rules.load_from_json(j.at("rules"));
    }
    if (j.count("title_block_values")) {
        const json &o = j["title_block_values"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            block->project_meta[it.key()] = it.value();
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

Schematic Schematic::new_from_file(const std::string &filename, Block &block, IPool &pool)
{
    auto j = load_json_from_file(filename);
    return Schematic(UUID(j.at("uuid").get<std::string>()), j, block, pool);
}

Schematic::Schematic(const UUID &uu, Block &bl) : uuid(uu), block(&bl)
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
                && (it_junc->second.net || it_junc->second.connection_count > 0)) {
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

void Schematic::disconnect_symbol(Sheet *sheet, SchematicSymbol *sym)
{
    assert(sheet == &sheets.at(sheet->uuid));
    assert(sym == &sheet->symbols.at(sym->uuid));
    std::map<const SymbolPin *, Junction *> pin_junctions;
    for (auto &it_line : sheet->net_lines) {
        LineNet *line = &it_line.second;
        // if((line->to_symbol && line->to_symbol->uuid == it.uuid) ||
        // (line->from_symbol &&line->from_symbol->uuid == it.uuid)) {
        for (auto it_ft : {&line->to, &line->from}) {
            if (it_ft->symbol == sym) {
                Junction *j = nullptr;
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
                if (it_ft.symbol->component->connections.count(conn_path)
                    && it_ft.pin->connected_net_lines.size() <= 1) {
                    it_ft.symbol->component->connections.erase(conn_path);
                }
                // prevents error in update_refs
                it_ft.pin->connected_net_lines.erase(line->uuid);
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
            for (auto it : sheet->net_lines) {
                auto &li = it.second;
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
                delete_net_line(sheet, line);
                line = nullptr;
                expand(true);
                from.update_refs(*sheet);
                to.update_refs(*sheet);

                UUID from_net_segment = from.get_net_segment();
                UUID to_net_segment = to.get_net_segment();
                auto ns_info = sheet->analyze_net_segments(false);
                Net *net_from = nullptr;
                Net *net_to = nullptr;
                if (ns_info.count(from_net_segment))
                    net_from = ns_info.at(from_net_segment).net;
                else if (from.is_bus_ripper())
                    net_from = from.bus_ripper->bus_member->net;

                if (ns_info.count(to_net_segment))
                    net_to = ns_info.at(to_net_segment).net;
                else if (to.is_bus_ripper())
                    net_to = to.bus_ripper->bus_member->net;

                // normal pin1-from pin2-to
                // swapped pin1-to pin2-from
                auto dst_normal = (pin1_pos - from.get_position()).mag_sq() + (pin2_pos - to.get_position()).mag_sq();
                auto dst_swapped = (pin2_pos - from.get_position()).mag_sq() + (pin1_pos - to.get_position()).mag_sq();
                SymbolPin *pin_from = &pin1;
                SymbolPin *pin_to = &pin2;
                if (dst_swapped < dst_normal) {
                    std::swap(pin_from, pin_to);
                }
                auto connect_pin = [this, sheet, sym](SymbolPin &pin, const LineNet::Connection &conn, Net *net) {
                    bool new_net = !net;
                    if (!net)
                        net = block->insert_net();

                    auto uu = UUID::random();
                    auto &new_line = sheet->net_lines.emplace(uu, uu).first->second;
                    new_line.from = conn;
                    new_line.to.connect(sym, &pin);
                    new_line.net = net;
                    sym->component->connections.emplace(std::piecewise_construct,
                                                        std::forward_as_tuple(sym->gate->uuid, pin.uuid),
                                                        std::forward_as_tuple(net));
                    if (conn.is_pin() && new_net) {
                        conn.symbol->component->connections.emplace(
                                std::piecewise_construct,
                                std::forward_as_tuple(conn.symbol->gate->uuid, conn.pin->uuid),
                                std::forward_as_tuple(net));
                    }
                };
                connect_pin(*pin_from, from, net_from);
                connect_pin(*pin_to, to, net_to);
                placed = true;
            }
        }
    }
    return placed;
}

void Schematic::expand(bool careful)
{
    for (auto &it_net : block->nets) {
        it_net.second.is_power_forced = false;
        it_net.second.is_bussed = false;
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
        for (auto &it : sheet.texts) {
            it.second.overridden = false;
        }
    }

    if (!careful) {
        block->vacuum_nets();
        block->vacuum_group_tag_names();
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
            sheet.warnings.clear();

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


    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        for (auto &it_sym : sheet.symbols) {
            auto &texts = it_sym.second.texts;
            texts.erase(std::remove_if(texts.begin(), texts.end(),
                                       [&sheet](const auto &a) { return sheet.texts.count(a.uuid) == 0; }),
                        texts.end());
        }
        if (!careful)
            sheet.expand_symbols(*this);
    }


    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;

        for (auto &it_junc : sheet.junctions) {
            it_junc.second.net = nullptr;
            it_junc.second.bus = nullptr;
        }
        for (auto &it_line : sheet.net_lines) {
            it_line.second.net = nullptr;
            it_line.second.bus = nullptr;
        }

        sheet.delete_dependants();
        if (!careful) {
            sheet.simplify_net_lines(false);
        }
        sheet.propagate_net_segments();
        if (!careful)
            sheet.vacuum_junctions();

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
    }


    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        if (!careful) {
            sheet.fix_junctions();
            sheet.delete_duplicate_net_lines();
            sheet.simplify_net_lines(true);
        }
        sheet.propagate_net_segments();
        sheet.analyze_net_segments(true);
    }
    update_refs();

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

                for (auto &it_line : sheet.net_lines) {
                    LineNet &line = it_line.second;
                    if (line.coord_on_line(pin_pos)) {
                        if (!line.is_connected_to(it_sym.first, it_pin.first)) {
                            sheet.warnings.emplace_back(pin_pos, "Pin on Line");
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
    std::map<std::string, std::set<const Net *>> net_names;
    for (const auto &it : block->nets) {
        if (it.second.is_named()) {
            net_names[it.second.name];
            net_names[it.second.name].insert(&it.second);
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

    for (auto &it_sheet : sheets) {
        Sheet &sheet = it_sheet.second;
        if (sheet.pool_frame) {
            std::map<std::string, std::string> values = block->project_meta;
            for (const auto &it_v : sheet.title_block_values) {
                values[it_v.first] = it_v.second;
            }
            sheet.frame = *sheet.pool_frame;
            for (auto &it_text : sheet.frame.texts) {
                auto &txt = it_text.second.text;
                replace_substr(txt, "$sheet_idx", std::to_string(sheet.index));
                replace_substr(txt, "$sheet_total", std::to_string(sheets.size()));
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

void Schematic::annotate()
{
    if (!annotation.keep) {
        for (auto &it : block->components) {
            it.second.refdes = "?";
        }
    }

    std::map<std::string, std::vector<unsigned int>> refdes;
    for (const auto &it : block->components) {
        refdes[it.second.entity->prefix];
    }


    std::vector<Sheet *> sheets_sorted;
    sheets_sorted.reserve(sheets.size());
    std::transform(sheets.begin(), sheets.end(), std::back_inserter(sheets_sorted), [](auto &a) { return &a.second; });
    std::sort(sheets_sorted.begin(), sheets_sorted.end(),
              [](const auto a, const auto b) { return a->index < b->index; });


    unsigned int sheet_offset = 0;
    unsigned int sheet_incr = 0;

    for (auto sheet : sheets_sorted) {
        if (annotation.mode != Annotation::Mode::SEQUENTIAL) {
            for (auto &it : refdes) {
                it.second.clear();
            }
            if (annotation.mode == Annotation::Mode::SHEET_100) {
                sheet_offset = 100 * sheet->index;
                sheet_incr = 100;
            }
            else if (annotation.mode == Annotation::Mode::SHEET_1000) {
                sheet_offset = 1000 * sheet->index;
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
            auto &v = refdes[sym->component->entity->prefix];
            if (sym->component->refdes.find('?') == std::string::npos) { // already annotated
                auto ss = sym->component->refdes.substr(sym->component->entity->prefix.size());
                int si = -1;
                try {
                    si = std::stoi(ss);
                }
                catch (const std::invalid_argument &e) {
                    if (!annotation.ignore_unknown)
                        sym->component->refdes = "?";
                }
                if (si > 0) {
                    v.push_back(si);
                    std::sort(v.begin(), v.end());
                }
            }
        }

        for (const auto sym : symbols) {
            auto &v = refdes[sym->component->entity->prefix];
            if (sym->component->refdes.find('?') != std::string::npos) { // needs annotatation
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
                if (sheet_incr && n / sheet_incr != sheet->index) {
                    n = sheet_offset + 1;
                }
                v.push_back(n);
                sym->component->refdes = sym->component->entity->prefix + std::to_string(n);
            }
            std::sort(v.begin(), v.end());
        }

        // for(auto &it_sym: it_sheet.second.symbols) {
        //
        //}
    }
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
            if (it_sym.second.gate->uuid == g1_uu) {
                it_sym.second.gate = &entity->gates.at(g2_uu);
            }
            else if (it_sym.second.gate->uuid == g2_uu) {
                it_sym.second.gate = &entity->gates.at(g1_uu);
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
      group_tag_visible(sch.group_tag_visible), annotation(sch.annotation), pdf_export_settings(sch.pdf_export_settings)
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
            for (auto &it_pin : schsym.symbol.pins) {
                for (auto &it_conn : it_pin.second.connected_net_lines) {
                    it_conn.second = &sheet.net_lines.at(it_conn.first);
                }
            }
            for (auto &it_text : schsym.texts) {
                it_text.update(sheet.texts);
            }
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
    }
}

json Schematic::serialize() const
{
    json j;
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


void Schematic::save_pictures(const std::string &dir) const
{
    std::list<const std::map<UUID, Picture> *> pictures;
    for (const auto &it : sheets) {
        pictures.push_back(&it.second.pictures);
    }
    pictures_save(pictures, dir, "sch");
}

void Schematic::load_pictures(const std::string &dir)
{
    std::list<std::map<UUID, Picture> *> pictures;
    for (auto &it : sheets) {
        pictures.push_back(&it.second.pictures);
    }
    pictures_load(pictures, dir, "sch");
}

} // namespace horizon
