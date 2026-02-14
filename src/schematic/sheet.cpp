#include "sheet.hpp"
#include "pool/part.hpp"
#include "pool/ipool.hpp"
#include "util/util.hpp"
#include "logger/logger.hpp"
#include "logger/log_util.hpp"
#include "common/object_descr.hpp"
#include "nlohmann/json.hpp"
#include "common/junction_util.hpp"

namespace horizon {

Sheet::Sheet(const UUID &uu, const json &j, Block &block, IPool &pool, IBlockSymbolAndSchematicProvider &prv)
    : uuid(uu), name(j.at("name").get<std::string>()), index(j.at("index").get<unsigned int>()), frame(UUID())
{
    Logger::log_info("loading sheet " + name, Logger::Domain::SCHEMATIC);
    if (j.count("frame")) {
        const auto &v = j.at("frame");
        if (v.is_string()) {
            pool_frame = pool.get_frame(UUID(v.get<std::string>()));
            frame = *pool_frame;
        }
    }
    {
        const json &o = j["junctions"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(junctions, ObjectType::JUNCTION, std::forward_as_tuple(u, it.value()),
                         Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["symbols"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(symbols, ObjectType::SCHEMATIC_SYMBOL, std::forward_as_tuple(u, it.value(), pool, &block),
                         Logger::Domain::SCHEMATIC);
        }
    }
    if (j.count("block_symbols")) {
        const json &o = j["block_symbols"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(block_symbols, ObjectType::SCHEMATIC_BLOCK_SYMBOL,
                         std::forward_as_tuple(u, it.value(), prv, block), Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["texts"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(texts, ObjectType::TEXT, std::forward_as_tuple(u, it.value()), Logger::Domain::SCHEMATIC);
        }
    }
    if (j.count("tables")) {
        const json &o = j["tables"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(tables, ObjectType::TABLE, std::forward_as_tuple(u, it.value()), Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["net_labels"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(net_labels, ObjectType::NET_LABEL, std::forward_as_tuple(u, it.value(), this),
                         Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["power_symbols"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(power_symbols, ObjectType::POWER_SYMBOL, std::forward_as_tuple(u, it.value(), this, &block),
                         Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["bus_rippers"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(bus_rippers, ObjectType::BUS_RIPPER, std::forward_as_tuple(u, it.value(), *this, block),
                         Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["net_lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(net_lines, ObjectType::LINE_NET, std::forward_as_tuple(u, it.value(), this),
                         Logger::Domain::SCHEMATIC);
        }
    }
    {
        const json &o = j["bus_labels"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            bus_labels.emplace(std::make_pair(u, BusLabel(u, it.value(), *this, block)));
        }
    }
    if (j.count("lines")) {
        const json &o = j["lines"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(lines, ObjectType::LINE, std::forward_as_tuple(u, it.value(), *this),
                         Logger::Domain::SCHEMATIC);
        }
    }
    if (j.count("arcs")) {
        const json &o = j["arcs"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(arcs, ObjectType::ARC, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::SCHEMATIC);
        }
    }
    if (j.count("pictures")) {
        const json &o = j["pictures"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(pictures, ObjectType::PICTURE, std::forward_as_tuple(u, it.value()),
                         Logger::Domain::SCHEMATIC);
        }
    }
    if (j.count("net_ties")) {
        const json &o = j["net_ties"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(net_ties, ObjectType::SCHEMATIC_NET_TIE, std::forward_as_tuple(u, it.value(), *this, block),
                         Logger::Domain::SCHEMATIC);
        }
    }

    if (j.count("title_block_values")) {
        const json &o = j["title_block_values"];
        for (const auto &[key, value] : o.items()) {
            title_block_values[key] = value.get<std::string>();
        }
    }
}

Sheet::Sheet(const UUID &uu) : uuid(uu), name("First sheet"), index(1), frame(UUID())
{
}

LineNet *Sheet::split_line_net(LineNet *it, SchematicJunction *ju)
{
    auto uu = UUID::random();
    LineNet *li = &net_lines.emplace(std::make_pair(uu, uu)).first->second;
    li->from.connect(ju);
    li->to = it->to;
    li->net = it->net;
    li->bus = it->bus;
    li->net_segment = it->net_segment;

    it->to.connect(ju);
    return li;
}

void Sheet::merge_net_lines(SchematicJunction &ju)
{
    auto &a = net_lines.at(ju.connected_net_lines.at(0));
    auto &b = net_lines.at(ju.connected_net_lines.at(1));
    if (a.from.junc == &ju) {
        if (b.from.junc == &ju) {
            a.from = b.to;
        }
        else {
            a.from = b.from;
        }
    }
    else {
        assert(a.to.junc == &ju);
        if (b.from.junc == &ju) {
            a.to = b.to;
        }
        else {
            a.to = b.from;
        }
    }

    // delete junction
    // delete b
    junctions.erase(ju.uuid);
    net_lines.erase(b.uuid);
    if (a.from.is_junc() && a.to.is_junc() && a.from.junc == a.to.junc) {
        net_lines.erase(a.uuid);
    }
}

void Sheet::expand_symbol_without_net_lines(const UUID &sym_uuid, const Schematic &sch,
                                            const BlockInstanceMapping *inst_map)
{
    auto &schsym = symbols.at(sym_uuid);
    if (schsym.symbol.unit->uuid != schsym.gate->unit->uuid) {
        throw std::logic_error("unit mismatch");
    }
    schsym.symbol = *schsym.pool_symbol;
    schsym.symbol.expand();
    schsym.apply_expand();
    if (!schsym.display_directions) {
        for (auto &it_pin : schsym.symbol.pins) {
            it_pin.second.direction = Pin::Direction::PASSIVE;
        }
    }
    schsym.symbol.apply_placement(schsym.placement);

    schsym.apply_pin_names();

    if (schsym.component->part) {
        for (auto &it_pin : schsym.symbol.pins) {
            it_pin.second.pad = "";
        }
        std::map<SymbolPin *, std::vector<std::string>> pads;
        for (const auto &it_pad_map : schsym.component->part->pad_map) {
            if (it_pad_map.second.gate == schsym.gate) {
                if (schsym.symbol.pins.count(it_pad_map.second.pin->uuid)) {
                    pads[&schsym.symbol.pins.at(it_pad_map.second.pin->uuid)].push_back(
                            schsym.component->part->package->pads.at(it_pad_map.first).name);
                }
            }
        }
        for (auto &it_pin : pads) {
            std::sort(it_pin.second.begin(), it_pin.second.end(),
                      [](const auto &a, const auto &b) { return strcmp_natural(a, b) < 0; });
            if (it_pin.second.size() <= 3 || schsym.display_all_pads) {
                for (const auto &pad : it_pin.second) {
                    it_pin.first->pad += pad + " ";
                }
            }
            else {
                it_pin.first->pad = it_pin.second.front() + " ... " + it_pin.second.back();
            }
        }
    }
    for (auto &it_text : schsym.symbol.texts) {
        it_text.second.text = schsym.replace_text(it_text.second.text, nullptr, sch, inst_map);
    }

    for (auto &it_text : schsym.texts) {
        it_text->text_override = schsym.replace_text(it_text->text, &it_text->overridden, sch, inst_map);
    }
}

void Sheet::expand_block_symbol_without_net_lines(const UUID &sym_uuid, const Schematic &sch)
{
    auto &schsym = block_symbols.at(sym_uuid);
    schsym.symbol = *schsym.prv_symbol;
    schsym.symbol.expand();
    for (auto &it_text : schsym.symbol.texts) {
        it_text.second.text = schsym.replace_text(it_text.second.text, nullptr, sch);
    }
}

void Sheet::expand_symbols(const class Schematic &sch, const BlockInstanceMapping *inst_map)
{
    for (auto &it_sym : symbols) {
        expand_symbol_without_net_lines(it_sym.first, sch, inst_map);
    }
    for (auto &it_sym : block_symbols) {
        expand_block_symbol_without_net_lines(it_sym.first, sch);
    }
    for (auto &it_line : net_lines) {
        LineNet &line = it_line.second;
        line.update_refs(*this);
    }
}

void Sheet::expand_symbol(const UUID &sym_uuid, const Schematic &sch, const BlockInstanceMapping *inst_map)
{
    expand_symbol_without_net_lines(sym_uuid, sch, inst_map);
    for (auto &it_line : net_lines) {
        LineNet &line = it_line.second;
        line.update_refs(*this);
    }
}

void Sheet::expand_block_symbol(const UUID &sym_uuid, const Schematic &sch)
{
    expand_block_symbol_without_net_lines(sym_uuid, sch);
    for (auto &it_line : net_lines) {
        LineNet &line = it_line.second;
        line.update_refs(*this);
    }
}

void Sheet::vacuum_junctions()
{
    map_erase_if(junctions, [](auto &x) {
        return x.second.connected_net_lines.size() == 0 && x.second.only_net_lines_connected();
    });
}

void Sheet::update_junction_connections()
{
    for (auto &[uu, it] : junctions) {
        it.clear();
        it.connected_bus_labels.clear();
        it.connected_bus_rippers.clear();
        it.connected_net_lines.clear();
        it.connected_power_symbols.clear();
        it.connected_net_labels.clear();
        it.connected_net_ties.clear();
    }
    for (auto &[uu, it] : net_lines) {
        for (const auto &it_ft : {it.from, it.to}) {
            if (it_ft.is_junc()) {
                auto &ju = *it_ft.junc;
                ju.connected_net_lines.push_back(uu);
            }
        }
    }

    JunctionUtil::update(lines);
    JunctionUtil::update(arcs);

    for (auto &[uu, it] : bus_rippers) {
        it.junction->connected_bus_rippers.push_back(uu);
    }
    for (auto &[uu, it] : bus_labels) {
        it.junction->connected_bus_labels.push_back(uu);
    }
    for (auto &[uu, it] : net_labels) {
        it.junction->connected_net_labels.push_back(uu);
    }
    for (auto &[uu, it] : power_symbols) {
        it.junction->connected_power_symbols.push_back(uu);
    }
    for (auto &[uu, it] : net_ties) {
        it.from->connected_net_ties.push_back(uu);
        it.to->connected_net_ties.push_back(uu);
    }
}

void Sheet::update_bus_ripper_connections()
{
    for (auto &[uu, it] : bus_rippers) {
        it.connections.clear();
    }
    for (auto &[uu, it] : net_lines) {
        for (const auto &it_ft : {it.from, it.to}) {
            if (it_ft.is_bus_ripper()) {
                it_ft.bus_ripper->connections.push_back(uu);
            }
        }
    }
}

void Sheet::simplify_net_lines()
{
    unsigned int n_merged = 1;
    while (n_merged) {
        n_merged = 0;
        std::vector<UUID> junctions_to_maybe_merge;
        for (auto &[uu, it] : junctions) {
            if (it.connected_net_lines.size() == 2 && it.only_net_lines_connected()) {
                junctions_to_maybe_merge.push_back(uu);
            }
        }
        for (const auto &uu : junctions_to_maybe_merge) {
            if (junctions.count(uu)) {
                auto &ju = junctions.at(uu);
                if (ju.connected_net_lines.size() == 2) {
                    const auto &line_a = net_lines.at(ju.connected_net_lines.at(0));
                    const auto &line_b = net_lines.at(ju.connected_net_lines.at(1));
                    auto va = line_a.to.get_position() - line_a.from.get_position();
                    auto vb = line_b.to.get_position() - line_b.from.get_position();
                    if (va.cross(vb) == 0) {
                        merge_net_lines(ju);
                        n_merged++;
                        update_junction_connections();
                    }
                }
            }
        }
    }
}


void Sheet::fix_junctions()
{
    for (auto &it_junc : junctions) {
        auto ju = &it_junc.second;
        for (auto &it_li : net_lines) {
            auto li = &it_li.second;
            if (ju->net_segment == li->net_segment && li->from.junc != ju && li->to.junc != ju) {
                if (li->coord_on_line(ju->position)) {
                    split_line_net(li, ju);
                }
            }
        }
    }
}

void Sheet::delete_duplicate_net_lines()
{
    std::set<std::pair<LineNet::Connection, LineNet::Connection>> conns;
    map_erase_if(net_lines, [&conns](auto &li) {
        bool del = false;
        if (conns.emplace(li.second.from, li.second.to).second == false)
            del = true;
        if (conns.emplace(li.second.to, li.second.from).second == false)
            del = true;
        return del;
    });
}

void Sheet::delete_dependants()
{
    auto label_it = net_labels.begin();
    while (label_it != net_labels.end()) {
        if (junctions.count(label_it->second.junction.uuid) == 0) {
            label_it = net_labels.erase(label_it);
        }
        else {
            label_it++;
        }
    }
    auto bus_label_it = bus_labels.begin();
    while (bus_label_it != bus_labels.end()) {
        if (junctions.count(bus_label_it->second.junction.uuid) == 0) {
            bus_label_it = bus_labels.erase(bus_label_it);
        }
        else {
            bus_label_it++;
        }
    }
    auto ps_it = power_symbols.begin();
    while (ps_it != power_symbols.end()) {
        if (junctions.count(ps_it->second.junction.uuid) == 0) {
            ps_it = power_symbols.erase(ps_it);
        }
        else {
            ps_it++;
        }
    }
}

void Sheet::propagate_net_segments()
{
    for (auto &it : junctions) {
        it.second.net_segment = UUID();
    }
    for (auto &it : bus_rippers) {
        it.second.net_segment = UUID();
    }
    for (auto &it : net_lines) {
        it.second.net_segment = UUID();
    }
    for (auto &it : symbols) {
        for (auto &it_pin : it.second.symbol.pins) {
            it_pin.second.net_segment = UUID();
            it_pin.second.connection_count = 0;
        }
    }
    for (auto &[uu_sym, sym] : block_symbols) {
        for (auto &[uu_port, port] : sym.symbol.ports) {
            port.net_segment = UUID();
            port.connection_count = 0;
        }
    }
    unsigned int run = 1;
    while (run) {
        run = 0;
        for (auto &it : net_lines) {
            if (!it.second.net_segment) {
                it.second.net_segment = UUID::random();
                run = 1;
                break;
            }
        }
        if (run == 0) {
            break;
        }
        unsigned int n_assigned = 1;
        while (n_assigned) {
            n_assigned = 0;
            for (auto &it : net_lines) {
                if (it.second.net_segment) { // net line with uuid
                    for (auto &it_ft : {it.second.from, it.second.to}) {
                        if (it_ft.is_junc() && !it_ft.junc->net_segment) {
                            it_ft.junc->net_segment = it.second.net_segment;
                            n_assigned++;
                        }
                        else if (it_ft.is_pin() && !it_ft.pin->net_segment) {
                            it_ft.pin->net_segment = it.second.net_segment;
                            it_ft.pin->connection_count++;
                            n_assigned++;
                        }
                        else if (it_ft.is_port() && !it_ft.port->net_segment) {
                            it_ft.port->net_segment = it.second.net_segment;
                            it_ft.port->connection_count++;
                            n_assigned++;
                        }
                        else if (it_ft.is_bus_ripper() && !it_ft.bus_ripper->net_segment) {
                            it_ft.bus_ripper->net_segment = it.second.net_segment;
                            n_assigned++;
                        }
                    }
                }
                else { // net line without uuid
                    for (auto &it_ft : {it.second.from, it.second.to}) {
                        if (it_ft.is_junc() && it_ft.junc->net_segment) {
                            it.second.net_segment = it_ft.junc->net_segment;
                            n_assigned++;
                        }
                        else if (it_ft.is_pin() && it_ft.pin->net_segment) {
                            it.second.net_segment = it_ft.pin->net_segment;
                            it_ft.pin->connection_count++;
                            n_assigned++;
                        }
                        else if (it_ft.is_port() && it_ft.port->net_segment) {
                            it.second.net_segment = it_ft.port->net_segment;
                            it_ft.port->connection_count++;
                            n_assigned++;
                        }
                        else if (it_ft.is_bus_ripper() && it_ft.bus_ripper->net_segment) {
                            it.second.net_segment = it_ft.bus_ripper->net_segment;
                            n_assigned++;
                        }
                    }
                }
            }
        }
    }
    // fix single junctions
    for (auto &it : junctions) {
        if (!it.second.net_segment) {
            it.second.net_segment = UUID::random();
        }
    }
}

NetSegmentInfo::NetSegmentInfo(const SchematicJunction *ju) : position(ju->position), net(ju->net), bus(ju->bus)
{
}
NetSegmentInfo::NetSegmentInfo(const LineNet *li)
    : position((li->to.get_position() + li->from.get_position()) / 2), net(li->net), bus(li->bus)
{
}
bool NetSegmentInfo::is_bus() const
{
    if (bus) {
        assert(!net);
        return true;
    }
    return false;
}

bool NetSegmentInfo::really_has_label() const
{
    return has_label && !has_power_sym && !has_bus_ripper;
}

std::map<UUID, NetSegmentInfo> Sheet::analyze_net_segments() const
{
    std::map<UUID, NetSegmentInfo> net_segments;
    for (auto &it : net_lines) {
        auto ns = it.second.net_segment;
        net_segments.emplace(ns, NetSegmentInfo(&it.second));
    }
    for (auto &it : junctions) {
        auto ns = it.second.net_segment;
        net_segments.emplace(ns, NetSegmentInfo(&it.second));
    }
    for (const auto &it : net_labels) {
        if (net_segments.count(it.second.junction->net_segment)) {
            net_segments.at(it.second.junction->net_segment).has_label = true;
        }
    }
    for (const auto &it : power_symbols) {
        if (net_segments.count(it.second.junction->net_segment)) {
            net_segments.at(it.second.junction->net_segment).has_power_sym = true;
            net_segments.at(it.second.junction->net_segment).has_label = true;
        }
    }
    for (const auto &it : bus_rippers) {
        if (net_segments.count(it.second.net_segment)) {
            net_segments.at(it.second.net_segment).has_label = true;
            net_segments.at(it.second.net_segment).has_bus_ripper = true;
        }
    }

    return net_segments;
}

void Sheet::place_warnings(const std::map<UUID, NetSegmentInfo> &net_segments)
{
    for (const auto &it : net_segments) {
        if (!it.second.has_label) {
            // std::cout<< "ns no label" << (std::string)it.second.net <<
            // std::endl;
            if (it.second.net && it.second.net->is_named()) {
                warnings.emplace_back(it.second.position, "Label missing");
            }
        }
        if (!it.second.has_power_sym) {
            // std::cout<< "ns no label" << (std::string)it.second.net <<
            // std::endl;
            if (it.second.net && it.second.net->is_power) {
                warnings.emplace_back(it.second.position, "Power sym missing");
            }
        }
    }
    std::set<Net *> nets_unnamed, ambigous_nets;

    for (const auto &it : net_segments) {
        if (it.second.net && !it.second.net->is_named()) {
            auto x = nets_unnamed.emplace(it.second.net);
            if (x.second == false) { // already exists
                ambigous_nets.emplace(it.second.net);
            }
        }
    }
    for (const auto &it : net_segments) {
        if (ambigous_nets.count(it.second.net)) {
            if (!it.second.has_label)
                warnings.emplace_back(it.second.position, "Ambigous nets");
        }
    }
    for (const auto &it : net_lines) {
        if (it.second.from.get_position() == it.second.to.get_position()) {
            warnings.emplace_back(it.second.from.get_position(), "Zero length line");
        }
    }
    for (const auto &[uu, tie] : net_ties) {
        const auto center = (tie.from->position + tie.to->position) / 2;
        if (tie.from->position == tie.to->position)
            warnings.emplace_back(center, "Zero length net tie");
        if (tie.from->net == tie.net_tie->net_primary) {
            // to must be secondary
            if (tie.to->net != tie.net_tie->net_secondary)
                warnings.emplace_back(center, "Net tie connected to incorrect net");
        }
        else if (tie.to->net == tie.net_tie->net_primary) {
            if (tie.from->net != tie.net_tie->net_secondary)
                warnings.emplace_back(center, "Net tie connected to incorrect net");
        }
        else {
            warnings.emplace_back(center, "Net tie connected to incorrect net");
        }
    }
}

Block::NetPinsAndPorts Sheet::get_pins_connected_to_net_segment(const UUID &uu_segment)
{
    Block::NetPinsAndPorts r;
    if (!uu_segment)
        return r;
    for (const auto &it_sym : symbols) {
        for (const auto &it_pin : it_sym.second.symbol.pins) {
            if (it_pin.second.net_segment == uu_segment) {
                r.pins.emplace(it_sym.second.component->uuid, it_sym.second.gate->uuid, it_pin.first);
            }
        }
    }
    for (const auto &[uu_sym, sym] : block_symbols) {
        for (const auto &[port_uu, port] : sym.symbol.ports) {
            if (port.net_segment == uu_segment) {
                r.ports.emplace(sym.block_instance->uuid, port.net);
            }
        }
    }
    return r;
}

bool Sheet::replace_junction(SchematicJunction *j, SchematicSymbol *sym, SymbolPin *pin)
{
    bool connected = false;
    for (auto &it_line : net_lines) {
        for (auto it_ft : {&it_line.second.from, &it_line.second.to}) {
            if (it_ft->junc == j) {
                it_ft->connect(sym, pin);
                connected = true;
            }
        }
    }
    return connected;
}

bool Sheet::replace_junction_or_create_line(SchematicJunction *j, SchematicSymbol *sym, SymbolPin *pin)
{
    if (!replace_junction(j, sym, pin)) {
        const auto li_uu = UUID::random();
        auto &li = net_lines.emplace(li_uu, li_uu).first->second;
        li.from.connect(j);
        li.to.connect(sym, pin);
        return false;
    }
    return true;
}


bool Sheet::replace_junction(SchematicJunction *j, SchematicBlockSymbol *sym, BlockSymbolPort *port)
{
    bool connected = false;
    for (auto &it_line : net_lines) {
        for (auto it_ft : {&it_line.second.from, &it_line.second.to}) {
            if (it_ft->junc == j) {
                it_ft->connect(sym, port);
                connected = true;
            }
        }
    }
    return connected;
}

bool Sheet::replace_junction_or_create_line(SchematicJunction *j, SchematicBlockSymbol *sym, BlockSymbolPort *port)
{
    if (!replace_junction(j, sym, port)) {
        const auto li_uu = UUID::random();
        auto &li = net_lines.emplace(li_uu, li_uu).first->second;
        li.from.connect(j);
        li.to.connect(sym, port);
        return false;
    }
    return true;
}

void Sheet::merge_junction(SchematicJunction *j, SchematicJunction *into)
{
    for (auto &it : net_lines) {
        if (it.second.from.junc == j) {
            it.second.from.junc = into;
        }
        if (it.second.to.junc == j) {
            it.second.to.junc = into;
        }
    }
    for (auto &it : net_labels) {
        if (it.second.junction == j)
            it.second.junction = into;
    }
    for (auto &it : bus_labels) {
        if (it.second.junction == j)
            it.second.junction = into;
    }
    for (auto &it : bus_rippers) {
        if (it.second.junction == j)
            it.second.junction = into;
    }
    for (auto &it : power_symbols) {
        if (it.second.junction == j)
            it.second.junction = into;
    }
    for (auto &it : lines) {
        if (it.second.to == j)
            it.second.to = into;
        if (it.second.from == j)
            it.second.from = into;
    }
    for (auto &it : arcs) {
        if (it.second.to == j)
            it.second.to = into;
        if (it.second.from == j)
            it.second.from = into;
        if (it.second.center == j)
            it.second.center = into;
    }
    junctions.erase(j->uuid);
}

SchematicJunction &Sheet::replace_bus_ripper(BusRipper &rip)
{
    auto uu = UUID::random();
    auto &j = junctions.emplace(uu, uu).first->second;
    j.net = rip.bus_member->net;
    j.position = rip.get_connector_pos();

    for (auto &it : rip.connections) {
        auto &line = net_lines.at(it);
        for (auto it_ft : {&line.from, &line.to}) {
            if (it_ft->bus_ripper == &rip) {
                it_ft->connect(&j);
            }
        }
    }
    return j;
}

Junction *Sheet::get_junction(const UUID &uu)
{
    return &junctions.at(uu);
}

bool Sheet::can_be_removed() const
{
    return symbols.size() == 0 && block_symbols.size() == 0;
}

template <typename T, typename U> static std::vector<T *> sort_symbols(U &sh)
{
    std::vector<T *> symbols;
    symbols.reserve(sh.block_symbols.size());
    for (auto &[uu, it] : sh.block_symbols) {
        symbols.push_back(&it);
    }
    std::sort(symbols.begin(), symbols.end(),
              [](auto a, auto b) { return strcmp_natural(a->block_instance->refdes, b->block_instance->refdes) < 0; });
    return symbols;
}

std::vector<SchematicBlockSymbol *> Sheet::get_block_symbols_sorted()
{
    return sort_symbols<SchematicBlockSymbol>(*this);
}

std::vector<const SchematicBlockSymbol *> Sheet::get_block_symbols_sorted() const
{
    return sort_symbols<const SchematicBlockSymbol>(*this);
}

json Sheet::serialize() const
{
    json j;
    j["name"] = name;
    j["index"] = index;
    if (frame.uuid)
        j["frame"] = (std::string)frame.uuid;
    j["symbols"] = json::object();
    j["title_block_values"] = title_block_values;

    for (const auto &it : symbols) {
        j["symbols"][(std::string)it.first] = it.second.serialize();
    }

    j["junctions"] = json::object();
    for (const auto &it : junctions) {
        j["junctions"][(std::string)it.first] = it.second.serialize();
    }
    j["net_lines"] = json::object();
    for (const auto &it : net_lines) {
        j["net_lines"][(std::string)it.first] = it.second.serialize();
    }
    j["texts"] = json::object();
    for (const auto &it : texts) {
        j["texts"][(std::string)it.first] = it.second.serialize();
    }
    j["tables"] = json::object();
    for (const auto &it : tables) {
        j["tables"][(std::string)it.first] = it.second.serialize();
    }
    j["net_labels"] = json::object();
    for (const auto &it : net_labels) {
        j["net_labels"][(std::string)it.first] = it.second.serialize();
    }
    j["power_symbols"] = json::object();
    for (const auto &it : power_symbols) {
        j["power_symbols"][(std::string)it.first] = it.second.serialize();
    }
    j["bus_labels"] = json::object();
    for (const auto &it : bus_labels) {
        j["bus_labels"][(std::string)it.first] = it.second.serialize();
    }
    j["bus_rippers"] = json::object();
    for (const auto &it : bus_rippers) {
        j["bus_rippers"][(std::string)it.first] = it.second.serialize();
    }
    j["lines"] = json::object();
    for (const auto &it : lines) {
        j["lines"][(std::string)it.first] = it.second.serialize();
    }
    j["arcs"] = json::object();
    for (const auto &it : arcs) {
        j["arcs"][(std::string)it.first] = it.second.serialize();
    }
    if (pictures.size()) {
        j["pictures"] = json::object();
        for (const auto &it : pictures) {
            j["pictures"][(std::string)it.first] = it.second.serialize();
        }
    }
    j["block_symbols"] = json::object();
    for (const auto &it : block_symbols) {
        j["block_symbols"][(std::string)it.first] = it.second.serialize();
    }
    if (net_ties.size()) {
        j["net_ties"] = json::object();
        for (const auto &it : net_ties) {
            j["net_ties"][(std::string)it.first] = it.second.serialize();
        }
    }
    return j;
}
} // namespace horizon
