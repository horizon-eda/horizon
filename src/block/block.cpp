#include "block.hpp"
#include "logger/log_util.hpp"
#include "logger/logger.hpp"
#include <set>
#include "nlohmann/json.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "pool/ipool.hpp"

namespace horizon {

const size_t Block::max_instance_path_len = 10;

bool Block::instance_path_too_long(const UUIDVec &path, const char *funcname)
{
    if (path.size() > max_instance_path_len) {
        Logger::log_critical("instance path exceeeds max. length of " + std::to_string(max_instance_path_len),
                             Logger::Domain::BLOCK, funcname);
        return true;
    }
    return false;
}

Block::Block(const UUID &uu, const json &j, IPool &pool, class IBlockProvider &prv)
    : uuid(uu), name(j.at("name").get<std::string>())
{
    Logger::log_info("loading block " + name, Logger::Domain::BLOCK, (std::string)uuid);
    if (j.count("net_classes")) {
        const json &o = j["net_classes"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(net_classes, ObjectType::NET_CLASS, std::forward_as_tuple(u, it.value()),
                         Logger::Domain::BLOCK);
        }
    }
    UUID nc_default_uuid = j.at("net_class_default").get<std::string>();
    if (net_classes.count(nc_default_uuid))
        net_class_default = &net_classes.at(nc_default_uuid);
    else
        Logger::log_critical("default net class " + (std::string)nc_default_uuid + " not found", Logger::Domain::BLOCK);

    {
        const json &o = j["nets"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(nets, ObjectType::NET, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BLOCK);
        }
    }

    for (auto &it : nets) {
        it.second.diffpair.update(nets);
    }
    update_diffpairs();

    {
        const json &o = j["buses"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(buses, ObjectType::BUS, std::forward_as_tuple(u, it.value(), *this), Logger::Domain::BLOCK);
        }
    }
    {
        const json &o = j["components"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(components, ObjectType::COMPONENT, std::forward_as_tuple(u, it.value(), pool, this),
                         Logger::Domain::BLOCK);
        }
    }
    if (j.count("block_instances")) {
        const json &o = j["block_instances"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            load_and_log(block_instances, ObjectType::BLOCK_INSTANCE, std::forward_as_tuple(u, it.value(), prv, this),
                         Logger::Domain::BLOCK);
        }
    }
    if (j.count("block_instance_mappings")) {
        const json &o = j["block_instance_mappings"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = uuid_vec_from_string(it.key());
            block_instance_mappings.emplace(std::piecewise_construct, std::forward_as_tuple(u),
                                            std::forward_as_tuple(it.value()));
        }
    }
    if (j.count("group_names")) {
        for (const auto &[key, value] : j.at("group_names").items()) {
            group_names.emplace(key, value.get<std::string>());
        }
    }
    if (j.count("tag_names")) {
        for (const auto &[key, value] : j.at("tag_names").items()) {
            tag_names.emplace(key, value.get<std::string>());
        }
    }
    for (const auto &it : components) {
        if (it.second.tag && !tag_names.count(it.second.tag)) {
            tag_names[it.second.tag] = std::to_string(tag_names.size());
        }
        if (it.second.group && !group_names.count(it.second.group)) {
            group_names[it.second.group] = std::to_string(group_names.size());
        }
    }
    if (j.count("bom_export_settings")) {
        try {
            bom_export_settings = BOMExportSettings(j.at("bom_export_settings"), pool);
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load bom export settings", Logger::Domain::BLOCK, e.what());
        }
    }
    if (j.count("project_meta")) {
        for (const auto &[key, value] : j.at("project_meta").items()) {
            project_meta.emplace(key, value.get<std::string>());
        }
    }
}

void Block::update_diffpairs()
{
    for (auto &it : nets) {
        if (!it.second.diffpair_master)
            it.second.diffpair = nullptr;
    }
    for (auto &it : nets) {
        if (it.second.diffpair_master) {
            if (nets.count(it.second.diffpair.uuid))
                it.second.diffpair->diffpair = &it.second;
            else
                it.second.diffpair = nullptr;
        }
    }
}

Block::Block(const UUID &uu) : uuid(uu)
{
    auto nuu = UUID::random();
    net_classes.emplace(std::piecewise_construct, std::forward_as_tuple(nuu), std::forward_as_tuple(nuu));
    net_class_default = &net_classes.begin()->second;
}

Block Block::new_from_file(const std::string &filename, IPool &obj, class IBlockProvider &prv)
{
    auto j = load_json_from_file(filename);
    return Block(UUID(j.at("uuid").get<std::string>()), j, obj, prv);
}

Net *Block::get_net(const UUID &uu)
{
    if (nets.count(uu))
        return &nets.at(uu);
    return nullptr;
}

Net *Block::insert_net()
{
    auto uu = UUID::random();
    auto n = &nets.emplace(uu, uu).first->second;
    n->net_class = net_class_default;
    return n;
}

void Block::merge_nets(Net *net, Net *into)
{
    assert(net->uuid == nets.at(net->uuid).uuid);
    assert(into->uuid == nets.at(into->uuid).uuid);
    for (auto &it_comp : components) {
        for (auto &it_conn : it_comp.second.connections) {
            if (it_conn.second.net == net) {
                it_conn.second.net = into;
            }
        }
    }
    for (auto &it_inst : block_instances) {
        for (auto &it_conn : it_inst.second.connections) {
            if (it_conn.second.net == net) {
                it_conn.second.net = into;
            }
        }
    }
    nets.erase(net->uuid);
}

void Block::update_refs()
{
    for (auto &it_comp : components) {
        for (auto &it_conn : it_comp.second.connections) {
            it_conn.second.net.update(nets);
        }
    }
    for (auto &it_inst : block_instances) {
        for (auto &it_conn : it_inst.second.connections) {
            it_conn.second.net.update(nets);
        }
    }
    for (auto &it_bus : buses) {
        it_bus.second.update_refs(*this);
    }
    net_class_default.update(net_classes);
    for (auto &it_net : nets) {
        it_net.second.net_class.update(net_classes);
        it_net.second.diffpair.update(nets);
    }
}

void Block::vacuum_nets()
{
    std::set<UUID> nets_erase;
    for (const auto &it : nets) {
        if (!it.second.is_power && !it.second.is_port) { // don't vacuum power nets and ports
            nets_erase.emplace(it.first);
        }
    }
    for (const auto &it : buses) {
        for (const auto &it_mem : it.second.members) {
            nets_erase.erase(it_mem.second.net->uuid);
        }
    }
    for (const auto &it_comp : components) {
        for (const auto &it_conn : it_comp.second.connections) {
            nets_erase.erase(it_conn.second.net.uuid);
        }
    }
    for (const auto &it_inst : block_instances) {
        for (const auto &it_conn : it_inst.second.connections) {
            nets_erase.erase(it_conn.second.net.uuid);
        }
    }
    for (const auto &uu : nets_erase) {
        nets.erase(uu);
    }
}

void Block::vacuum_group_tag_names()
{
    std::set<UUID> groups;
    std::set<UUID> tags;
    for (const auto &it : components) {
        if (it.second.group) {
            groups.insert(it.second.group);
        }
        if (it.second.tag) {
            tags.insert(it.second.tag);
        }
    }
    map_erase_if(group_names, [&groups](const auto &a) { return groups.count(a.first) == 0; });
    map_erase_if(tag_names, [&tags](const auto &a) { return tags.count(a.first) == 0; });
}

void Block::update_connection_count()
{
    for (auto &it : nets) {
        it.second.n_pins_connected = 0;
        it.second.has_bus_rippers = false;
    }
    for (const auto &it_comp : components) {
        for (const auto &it_conn : it_comp.second.connections) {
            if (it_conn.second.net)
                it_conn.second.net->n_pins_connected++;
        }
    }
    for (const auto &it_inst : block_instances) {
        for (const auto &it_conn : it_inst.second.connections) {
            if (it_conn.second.net)
                it_conn.second.net->n_pins_connected++;
        }
    }
}

Block::Block(const Block &block)
    : uuid(block.uuid), name(block.name), nets(block.nets), buses(block.buses), components(block.components),
      block_instances(block.block_instances), net_classes(block.net_classes),
      net_class_default(block.net_class_default), block_instance_mappings(block.block_instance_mappings),
      group_names(block.group_names), tag_names(block.tag_names), project_meta(block.project_meta),
      bom_export_settings(block.bom_export_settings)
{
    update_refs();
}

void Block::operator=(const Block &block)
{
    uuid = block.uuid;
    name = block.name;
    nets = block.nets;
    buses = block.buses;
    components = block.components;
    block_instances = block.block_instances;
    net_classes = block.net_classes;
    net_class_default = block.net_class_default;
    block_instance_mappings = block.block_instance_mappings;
    group_names = block.group_names;
    tag_names = block.tag_names;
    project_meta = block.project_meta;
    bom_export_settings = block.bom_export_settings;
    update_refs();
}

json Block::serialize() const
{
    json j;
    j["name"] = name;
    j["uuid"] = (std::string)uuid;
    j["net_class_default"] = (std::string)net_class_default->uuid;
    j["nets"] = json::object();
    for (const auto &it : nets) {
        j["nets"][(std::string)it.first] = it.second.serialize();
    }
    j["components"] = json::object();
    for (const auto &it : components) {
        j["components"][(std::string)it.first] = it.second.serialize();
    }
    j["block_instances"] = json::object();
    for (const auto &it : block_instances) {
        j["block_instances"][(std::string)it.first] = it.second.serialize();
    }
    j["block_instance_mappings"] = json::object();
    for (const auto &it : block_instance_mappings) {
        j["block_instance_mappings"][uuid_vec_to_string(it.first)] = it.second.serialize();
    }
    j["buses"] = json::object();
    for (const auto &it : buses) {
        j["buses"][(std::string)it.first] = it.second.serialize();
    }
    j["net_classes"] = json::object();
    for (const auto &it : net_classes) {
        j["net_classes"][(std::string)it.first] = it.second.serialize();
    }
    j["group_names"] = json::object();
    for (const auto &it : group_names) {
        j["group_names"][(std::string)it.first] = it.second;
    }
    j["tag_names"] = json::object();
    for (const auto &it : tag_names) {
        j["tag_names"][(std::string)it.first] = it.second;
    }
    j["bom_export_settings"] = bom_export_settings.serialize();
    j["project_meta"] = project_meta;

    return j;
}

Net *Block::extract_pins(const NetPinsAndPorts &pins, Net *net)
{
    if (pins.pins.size() == 0 && pins.ports.size() == 0) {
        return nullptr;
    }
    if (net == nullptr) {
        net = insert_net();
    }
    for (const auto &it : pins.pins) {
        Component &comp = components.at(it.at(0));
        UUIDPath<2> conn_path(it.at(1), it.at(2));
        if (comp.connections.count(conn_path)) {
            // the connection may have been deleted
            comp.connections.at(conn_path).net = net;
        }
    }
    for (const auto &it : pins.ports) {
        auto &inst = block_instances.at(it.at(0));
        const auto net_port = it.at(1);
        if (inst.connections.count(net_port)) {
            // the connection may have been deleted
            inst.connections.at(net_port).net = net;
        }
    }

    return net;
}

BlockInstanceMapping::ComponentInfo Block::get_component_info(const Component &comp, const UUIDVec &instance_path) const
{
    if (instance_path.size()) {
        auto &comps = block_instance_mappings.at(instance_path).components;
        if (comps.count(comp.uuid))
            return comps.at(comp.uuid);
    }

    BlockInstanceMapping::ComponentInfo info;
    info.refdes = comp.refdes;
    info.nopopulate = comp.nopopulate;
    return info;
}

std::string Block::get_refdes(const Component &comp, const UUIDVec &instance_path) const
{
    return get_component_info(comp, instance_path).refdes;
}


void Block::set_refdes(Component &comp, const UUIDVec &instance_path, const std::string &rd)
{
    if (instance_path.size()) {
        block_instance_mappings.at(instance_path).components[comp.uuid].refdes = rd;
    }
    else {
        comp.refdes = rd;
    }
}

void Block::set_nopopulate(Component &comp, const UUIDVec &instance_path, bool np)
{
    if (instance_path.size()) {
        block_instance_mappings.at(instance_path).components[comp.uuid].nopopulate = np;
    }
    else {
        comp.nopopulate = np;
    }
}

using BOMRows = std::map<const Part *, BOMRow>;

BOMRows Block::get_BOM(const BOMExportSettings &settings) const
{
    BOMRows rows;
    for (const auto &[block, instance_path] : get_instantiated_blocks_and_top()) {
        for (const auto &[uu, comp] : block.components) {
            const auto comp_info = get_component_info(comp, instance_path);
            if (comp_info.nopopulate && !settings.include_nopopulate) {
                continue;
            }
            if (comp.part) {
                const Part *part;
                if (settings.concrete_parts.count(comp.part->uuid))
                    part = settings.concrete_parts.at(comp.part->uuid);
                else
                    part = comp.part;
                if (part->get_flag(Part::Flag::EXCLUDE_BOM))
                    continue;
                rows[part].refdes.push_back(comp_info.refdes);
            }
        }
    }

    for (auto &it : rows) {
        if (settings.orderable_MPNs.count(it.first->uuid)
            && it.first->orderable_MPNs.count(settings.orderable_MPNs.at(it.first->uuid)))
            it.second.MPN = it.first->orderable_MPNs.at(settings.orderable_MPNs.at(it.first->uuid));
        else
            it.second.MPN = it.first->get_MPN();

        it.second.datasheet = it.first->get_datasheet();
        it.second.description = it.first->get_description();
        it.second.manufacturer = it.first->get_manufacturer();
        it.second.package = it.first->package->name;
        it.second.value = it.first->get_value();
        std::sort(it.second.refdes.begin(), it.second.refdes.end(),
                  [](const auto &a, const auto &b) { return strcmp_natural(a, b) < 0; });
    }
    return rows;
}

std::string Block::get_group_name(const UUID &uu) const
{
    if (!uu)
        return "None";
    else if (group_names.count(uu))
        return group_names.at(uu);
    else
        return (std::string)uu;
}

std::string Block::get_tag_name(const UUID &uu) const
{
    if (!uu)
        return "None";
    else if (tag_names.count(uu))
        return tag_names.at(uu);
    else
        return (std::string)uu;
}

std::map<std::string, std::string> Block::peek_project_meta(const std::string &filename)
{
    auto j = load_json_from_file(filename);
    if (j.count("project_meta")) {
        const json &o = j["project_meta"];
        std::map<std::string, std::string> project_meta;
        for (const auto &[key, value] : o.items()) {
            project_meta.emplace(key, value.get<std::string>());
        }
        return project_meta;
    }
    return {};
}

std::set<UUID> Block::peek_instantiated_blocks(const std::string &filename)
{
    std::set<UUID> blocks;
    auto j = load_json_from_file(filename);
    if (j.count("block_instances")) {
        for (const auto &[key, value] : j.at("block_instances").items()) {
            blocks.emplace(value.at("block").get<std::string>());
        }
    }
    return blocks;
}


std::string Block::get_net_name(const UUID &uu) const
{
    auto &net = nets.at(uu);
    if (net.name.size()) {
        return net.name;
    }
    else {
        std::string n;
        for (const auto &it : components) {
            for (const auto &it_conn : it.second.connections) {
                if (it_conn.second.net && it_conn.second.net->uuid == uu) {
                    n += it.second.refdes + ", ";
                }
            }
        }
        if (n.size()) {
            n.pop_back();
            n.pop_back();
        }
        return n;
    }
}

bool Block::can_swap_gates(const UUID &comp_uu, const UUID &g1_uu, const UUID &g2_uu) const
{
    const auto &comp = components.at(comp_uu);
    const auto &g1 = comp.entity->gates.at(g1_uu);
    const auto &g2 = comp.entity->gates.at(g2_uu);
    return (g1.unit->uuid == g2.unit->uuid) && (g1.swap_group == g2.swap_group) && (g1.swap_group != 0);
}

void Block::swap_gates(const UUID &comp_uu, const UUID &g1_uu, const UUID &g2_uu)
{
    if (!can_swap_gates(comp_uu, g1_uu, g2_uu))
        throw std::runtime_error("can't swap gates");

    auto &comp = components.at(comp_uu);
    std::map<UUIDPath<2>, Connection> new_connections;
    for (const auto &it : comp.connections) {
        if (it.first.at(0) == g1_uu) {
            new_connections.emplace(std::piecewise_construct, std::forward_as_tuple(g2_uu, it.first.at(1)),
                                    std::forward_as_tuple(it.second));
        }
        else if (it.first.at(0) == g2_uu) {
            new_connections.emplace(std::piecewise_construct, std::forward_as_tuple(g1_uu, it.first.at(1)),
                                    std::forward_as_tuple(it.second));
        }
        else {
            new_connections.emplace(it);
        }
    }
    comp.connections = new_connections;
}

ItemSet Block::get_pool_items_used() const
{
    ItemSet items_needed;
    auto add_part = [&items_needed](const Part *part) {
        while (part) {
            items_needed.emplace(ObjectType::PART, part->uuid);
            items_needed.emplace(ObjectType::PACKAGE, part->package.uuid);
            for (const auto &it_pad : part->package->pads) {
                items_needed.emplace(ObjectType::PADSTACK, it_pad.second.pool_padstack->uuid);
            }
            part = part->base;
        }
    };

    for (const auto &it : components) {
        items_needed.emplace(ObjectType::ENTITY, it.second.entity->uuid);
        for (const auto &it_gate : it.second.entity->gates) {
            items_needed.emplace(ObjectType::UNIT, it_gate.second.unit->uuid);
        }
        if (it.second.part) {
            add_part(it.second.part);
        }
    }
    for (const auto &it : bom_export_settings.concrete_parts) {
        add_part(it.second);
    }
    return items_needed;
}

UUID Block::get_uuid() const
{
    return uuid;
}

bool Block::can_delete_power_net(const UUID &uu) const
{
    if (nets.count(uu)) {
        auto &bn = nets.at(uu);
        return bn.n_pins_connected == 0 && !bn.is_power_forced;
    }
    else {
        return true;
    }
}

void Block::update_non_top(Block &other) const
{
    other.project_meta = project_meta;

    {
        for (const auto &[uu, nc] : net_classes) {
            auto &nc_other = other.net_classes.emplace(uu, uu).first->second;
            nc_other.name = nc.name;
        }
        std::vector<UUID> nc_delete;
        for (const auto &[uu, nc] : other.net_classes) {
            if (!net_classes.count(uu)) {
                nc_delete.push_back(uu);
            }
        }
        for (const auto &uu : nc_delete) {
            if (std::any_of(other.nets.begin(), other.nets.end(),
                            [&uu](auto &x) { return x.second.net_class->uuid == uu; })) {
                // net class still in use
                Logger::log_warning("Net class " + other.net_classes.at(uu).name + " still in use in block "
                                            + other.name,
                                    Logger::Domain::BLOCK);
            }
            else {
                other.net_classes.erase(uu);
            }
        }
    }

    {
        for (const auto &[uu, net] : nets) {
            if (net.is_power) {
                auto &net_other = other.nets.emplace(uu, uu).first->second;
                net_other.is_power = true;
                net_other.power_symbol_style = net.power_symbol_style;
                net_other.power_symbol_name_visible = net.power_symbol_name_visible;
                net_other.name = net.name;
                net_other.net_class = &other.net_classes.at(net.net_class->uuid);
            }
        }
        std::vector<UUID> nets_delete;
        for (const auto &[uu, net] : other.nets) {
            if (net.is_power) {
                if (!nets.count(uu)) {
                    nets_delete.push_back(uu);
                }
            }
        }
        for (const auto &uu : nets_delete) {
            if (other.can_delete_power_net(uu)) {
                other.nets.erase(uu);
            }
            else {
                // net still in use
                Logger::log_warning("Power net " + other.nets.at(uu).name + " still in use in block " + other.name,
                                    Logger::Domain::BLOCK);
            }
        }
    }
}


template <bool c> using WalkCB = std::function<void(make_const_ref_t<c, Block>, const UUIDVec &)>;

template <bool c> struct WalkContext {
    WalkCB<c> cb;
    const Block &top;
    const bool inc_top;
};

template <bool c>
static void walk_blocks_rec(make_const_ref_t<c, Block> block, const UUIDVec &instance_path, WalkContext<c> &ctx)
{
    if (Block::instance_path_too_long(instance_path, __FUNCTION__))
        return;
    if (ctx.inc_top || instance_path.size())
        ctx.cb(block, instance_path);
    for (auto &[uu, inst] : block.block_instances) {
        walk_blocks_rec(*inst.block, uuid_vec_append(instance_path, uu), ctx);
    }
}

template <bool c> static void walk_blocks(make_const_ref_t<c, Block> top, WalkCB<c> cb, bool inc_top)
{
    WalkContext<c> ctx{cb, top, inc_top};
    walk_blocks_rec<c>(top, {}, ctx);
}

std::vector<Block::BlockItem<true>> Block::get_instantiated_blocks(bool inc_top) const
{
    std::vector<Block::BlockItem<true>> items;
    walk_blocks<true>(
            *this,
            [&items](const Block &block, const UUIDVec &instance_path) { items.emplace_back(block, instance_path); },
            inc_top);
    return items;
}

std::vector<Block::BlockItem<false>> Block::get_instantiated_blocks(bool inc_top)
{
    std::vector<Block::BlockItem<false>> items;
    walk_blocks<false>(
            *this, [&items](Block &block, const UUIDVec &instance_path) { items.emplace_back(block, instance_path); },
            inc_top);
    return items;
}

std::vector<Block::BlockItem<true>> Block::get_instantiated_blocks() const
{
    return get_instantiated_blocks(false);
}


std::vector<Block::BlockItem<false>> Block::get_instantiated_blocks()
{
    return get_instantiated_blocks(false);
}


std::vector<Block::BlockItem<true>> Block::get_instantiated_blocks_and_top() const
{
    return get_instantiated_blocks(true);
}


std::vector<Block::BlockItem<false>> Block::get_instantiated_blocks_and_top()
{
    return get_instantiated_blocks(true);
}

void Block::create_instance_mappings()
{
    for (const auto &[block, instance_path] : get_instantiated_blocks()) {
        block_instance_mappings.emplace(instance_path, block);
    };
}

static std::set<UUID> get_blocks_along_instance_path(const Block &top, const UUIDVec &instance_path)
{
    std::set<UUID> blocks;
    blocks.insert(top.uuid);
    const Block *block = &top;
    for (const auto &uu : instance_path) {
        block = block->block_instances.at(uu).block;
        blocks.insert(block->uuid);
    }
    return blocks;
}

static std::set<UUID> get_parent_blocks(const Block &top, const UUID &current)
{
    std::set<UUID> blocks;
    for (const auto &it : top.get_instantiated_blocks_and_top()) {
        if (it.block.uuid == current) {
            const auto x = get_blocks_along_instance_path(top, it.instance_path);
            blocks.insert(x.begin(), x.end());
        }
    }
    return blocks;
}

bool Block::can_add_block_instance(const UUID &where, const UUID &block_inst) const
{
    if (where == block_inst)
        return false;
    const auto blocks = get_parent_blocks(*this, where);
    return blocks.count(block_inst) == 0;
}

struct FlattenContext {
    Block &flat;
    const Block &top;
    std::map<UUIDVec, UUID> net_map;
    std::set<UUIDVec> nc_nets;
};


static void update_net_map_for_instance(const BlockInstance &inst, const UUIDVec &instance_path, FlattenContext &ctx)
{
    for (const auto &[port, conn] : inst.connections) {
        const auto inst_net = uuid_vec_append(uuid_vec_append(instance_path, inst.uuid), port);
        if (conn.net) {
            const auto flat_uu = ctx.net_map.at(uuid_vec_append(instance_path, conn.net->uuid));
            auto x = ctx.net_map.emplace(inst_net, flat_uu);
            assert(x.second);
            assert(ctx.flat.nets.count(flat_uu));
        }
        else {
            ctx.nc_nets.insert(inst_net);
        }
    }
}

static std::string instance_path_to_string(const UUIDVec &instance_path, const Block &top)
{
    std::string r;
    const Block *block = &top;
    for (const auto &uu : instance_path) {
        if (r.size())
            r += "/";
        const auto &inst = block->block_instances.at(uu);
        r += inst.refdes;
        block = inst.block;
    }
    return r;
}

static void visit_block_for_flatten(const Block &block, const UUIDVec &instance_path, FlattenContext &ctx)
{
    if (Block::instance_path_too_long(instance_path, __FUNCTION__))
        return;
    for (const auto &[uu, net] : block.nets) {
        const auto v = uuid_vec_append(instance_path, uu);
        if (net.is_power) {
            ctx.net_map.emplace(v, uu);
        }
        else {
            const auto flat_uu = uuid_vec_flatten(v);
            if (ctx.net_map.emplace(v, flat_uu).second) {
                auto &flat_net = ctx.flat.nets.emplace(flat_uu, flat_uu).first->second;
                flat_net.net_class = ctx.flat.net_class_default;
                flat_net.name = instance_path_to_string(instance_path, ctx.top) + "/" + block.get_net_name(uu);
                if (ctx.flat.net_classes.count(net.net_class->uuid)) {
                    flat_net.net_class = &ctx.flat.net_classes.at(net.net_class->uuid);
                }
                else {
                    Logger::log_warning("net class not found in flattend netlist", Logger::Domain::BLOCK);
                }
            }
        }
    }
    for (const auto &[uu, comp] : block.components) {
        const auto href = uuid_vec_append(instance_path, uu);
        const auto flat_uu = uuid_vec_flatten(href);
        auto &flat_comp = ctx.flat.components.emplace(flat_uu, flat_uu).first->second;
        flat_comp.entity = comp.entity;
        flat_comp.part = comp.part;
        const auto comp_info = ctx.top.get_component_info(comp, instance_path);
        flat_comp.refdes = comp_info.refdes;
        flat_comp.value = comp.value;
        flat_comp.group = uuid_vec_flatten(uuid_vec_append(instance_path, comp.group));
        flat_comp.tag = comp.tag;
        flat_comp.nopopulate = comp_info.nopopulate;
        flat_comp.href = href;
        for (const auto &[k, conn] : comp.connections) {
            Net *net = nullptr;
            if (conn.net)
                net = &ctx.flat.nets.at(ctx.net_map.at(uuid_vec_append(instance_path, conn.net->uuid)));
            flat_comp.connections.emplace(k, net);
        }
    }
    for (const auto &[uu, name] : block.group_names) {
        ctx.flat.group_names.emplace(uuid_vec_flatten(uuid_vec_append(instance_path, uu)),
                                     instance_path_to_string(instance_path, ctx.top) + "/" + name);
    }
    for (const auto &[uu, name] : block.tag_names) {
        ctx.flat.tag_names.emplace(uu, instance_path_to_string(instance_path, ctx.top) + "/" + name);
    }
    for (const auto &[uu, inst] : block.block_instances) {
        update_net_map_for_instance(inst, instance_path, ctx);
        visit_block_for_flatten(*inst.block, uuid_vec_append(instance_path, uu), ctx);
    }
}

Block Block::flatten() const
{
    Block flat = *this;
    flat.block_instances.clear();
    flat.block_instance_mappings.clear();
    for (auto &[uu, comp] : flat.components) {
        comp.href = {uu};
    }
    FlattenContext ctx{flat, *this};
    for (const auto &[uu, net] : nets) {
        ctx.net_map.emplace(UUIDVec{uu}, uu);
    }
    for (const auto &[uu, inst] : block_instances) {
        update_net_map_for_instance(inst, {}, ctx);
        visit_block_for_flatten(*inst.block, {inst.uuid}, ctx);
    }
    for (const auto &[path, uu] : ctx.net_map) {
        flat.nets.at(uu).hrefs.push_back(path);
    }
    for (const auto &net : ctx.nc_nets) {
        flat.nets.at(ctx.net_map.at(net)).is_nc = true;
    }
    return flat;
}


} // namespace horizon
