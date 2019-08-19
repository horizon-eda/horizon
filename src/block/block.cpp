#include "block.hpp"
#include "logger/log_util.hpp"
#include "logger/logger.hpp"
#include <set>
#include "nlohmann/json.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"

namespace horizon {

Block::Block(const UUID &uu, const json &j, Pool &pool) : uuid(uu), name(j.at("name").get<std::string>())
{
    Logger::log_info("loading block " + (std::string)uuid, Logger::Domain::BLOCK);
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
    if (j.count("group_names")) {
        const json &o = j["group_names"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            group_names[u] = it.value().get<std::string>();
        }
    }
    if (j.count("tag_names")) {
        const json &o = j["tag_names"];
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            auto u = UUID(it.key());
            tag_names[u] = it.value().get<std::string>();
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
            bom_export_settings = BOMExportSettings(j.at("bom_export_settings"));
        }
        catch (const std::exception &e) {
            Logger::log_warning("couldn't load bom export settings", Logger::Domain::BLOCK, e.what());
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

Block Block::new_from_file(const std::string &filename, Pool &obj)
{
    auto j = load_json_from_file(filename);
    return Block(UUID(j["uuid"].get<std::string>()), j, obj);
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
    nets.erase(net->uuid);
}

void Block::update_refs()
{
    for (auto &it_comp : components) {
        for (auto &it_conn : it_comp.second.connections) {
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
        if (!it.second.is_power) { // don't vacuum power nets
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
}

Block::Block(const Block &block)
    : uuid(block.uuid), name(block.name), nets(block.nets), buses(block.buses), components(block.components),
      net_classes(block.net_classes), net_class_default(block.net_class_default), group_names(block.group_names),
      tag_names(block.tag_names), bom_export_settings(block.bom_export_settings)
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
    net_classes = block.net_classes;
    net_class_default = block.net_class_default;
    group_names = block.group_names;
    tag_names = block.tag_names;
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

    return j;
}

Net *Block::extract_pins(const std::set<UUIDPath<3>> &pins, Net *net)
{
    if (pins.size() == 0) {
        return nullptr;
    }
    if (net == nullptr) {
        net = insert_net();
    }
    for (const auto &it : pins) {
        Component &comp = components.at(it.at(0));
        UUIDPath<2> conn_path(it.at(1), it.at(2));
        if (comp.connections.count(conn_path)) {
            // the connection may have been deleted
            comp.connections.at(conn_path).net = net;
        }
    }

    return net;
}

std::map<const Part *, BOMRow> Block::get_BOM(const BOMExportSettings &settings) const
{
    std::map<const Part *, BOMRow> rows;
    for (const auto &it : components) {
        if (it.second.part) {
            rows[it.second.part];
            rows[it.second.part].refdes.push_back(it.second.refdes);
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

} // namespace horizon
