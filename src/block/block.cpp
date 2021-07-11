#include "block.hpp"
#include "logger/log_util.hpp"
#include "logger/logger.hpp"
#include <set>
#include "nlohmann/json.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"
#include "pool/ipool.hpp"

namespace horizon {

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
            load_and_log(block_instances, ObjectType::BLOCK_INSTANCE, std::forward_as_tuple(u, it.value(), *this, prv),
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
        const json &o = j["group_names"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            group_names[u] = it.value();
        }
    }
    if (j.count("tag_names")) {
        const json &o = j["tag_names"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            tag_names[u] = it.value();
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
        const json &o = j["project_meta"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            project_meta[it.key()] = it.value();
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

json Block::serialize()
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

static BlockInstanceMapping::ComponentInfo get_component_info(const Component &comp, const UUIDVec &instance_path,
                                                              const Block &top)
{
    if (instance_path.size()) {
        auto &comps = top.block_instance_mappings.at(instance_path).components;
        if (comps.count(comp.uuid))
            return comps.at(comp.uuid);
    }

    BlockInstanceMapping::ComponentInfo info;
    info.refdes = comp.refdes;
    info.nopopulate = comp.nopopulate;
    return info;
}

using BOMRows = std::map<const Part *, BOMRow>;

struct BOMContext {
    const Block &top;
    const BOMExportSettings &settings;
    BOMRows &rows;
};

static void visit_block_for_bom(const Block &block, const UUIDVec &instance_path, BOMContext &ctx)
{
    for (const auto &[uu, comp] : block.components) {
        const auto comp_info = get_component_info(comp, instance_path, ctx.top);
        if (comp_info.nopopulate && !ctx.settings.include_nopopulate) {
            continue;
        }
        if (comp.part) {
            const Part *part;
            if (ctx.settings.concrete_parts.count(comp.part->uuid))
                part = ctx.settings.concrete_parts.at(comp.part->uuid);
            else
                part = comp.part;
            if (part->get_flag(Part::Flag::EXCLUDE_BOM))
                continue;
            ctx.rows[part].refdes.push_back(comp_info.refdes);
        }
    }
    for (const auto &[uu, inst] : block.block_instances) {
        visit_block_for_bom(*inst.block, uuid_vec_append(instance_path, inst.uuid), ctx);
    }
}

BOMRows Block::get_BOM(const BOMExportSettings &settings) const
{
    BOMRows rows;
    BOMContext ctx{*this, settings, rows};
    visit_block_for_bom(*this, {}, ctx);
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
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            project_meta[it.key()] = it.value();
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

void Block::update_non_top(Block &other) const
{
    other.project_meta = project_meta;
}


using WalkCB = std::function<void(const Block &, const UUIDVec &)>;

struct WalkContext {
    WalkCB cb;
    const Block &top;
};

static void walk_blocks_rec(const Block &block, const UUIDVec &instance_path, WalkContext &ctx)
{
    if (instance_path.size())
        ctx.cb(block, instance_path);
    for (auto &[uu, inst] : block.block_instances) {
        walk_blocks_rec(*inst.block, uuid_vec_append(instance_path, uu), ctx);
    }
}

static void walk_blocks(const Block &top, WalkCB cb)
{
    WalkContext ctx{cb, top};
    walk_blocks_rec(top, {}, ctx);
}

void Block::create_instance_mappings()
{
    walk_blocks(*this, [this](const Block &block, const UUIDVec &instance_path) {
        block_instance_mappings.emplace(instance_path, block);
    });
}

Block Block::flatten() const
{
    Block flat = *this;
    flat.block_instances.clear();
    flat.block_instance_mappings.clear();
    walk_blocks(*this, [this, &flat](const Block &block, const UUIDVec &instance_path) {
        for (const auto &[uu, comp] : block.components) {
            const auto flat_uu = uuid_vec_flatten(uuid_vec_append(instance_path, uu));
            auto &flat_comp = flat.components.emplace(flat_uu, flat_uu).first->second;
            flat_comp.entity = comp.entity;
            flat_comp.part = comp.part;
            const auto comp_info = get_component_info(comp, instance_path, *this);
            flat_comp.refdes = comp_info.refdes;
            flat_comp.value = comp.value;
            flat_comp.group = uuid_vec_flatten(uuid_vec_append(instance_path, comp.group));
            flat_comp.tag = uuid_vec_flatten(uuid_vec_append({block.uuid}, comp.tag));
            flat_comp.nopopulate = comp_info.nopopulate;
        }
    });
    return flat;
}


} // namespace horizon
