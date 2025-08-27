#include "block_instance.hpp"
#include "block.hpp"
#include "logger/logger.hpp"
#include <nlohmann/json.hpp>
#include "blocks/iblock_provider.hpp"

namespace horizon {

BlockInstance::BlockInstance(const UUID &uu, const json &j, IBlockProvider &prv, Block *bl)
    : uuid(uu), block(&prv.get_block(j.at("block").get<std::string>())), refdes(j.at("refdes").get<std::string>())
{
    {
        const json &o = j.at("connections");
        for (auto it = o.cbegin(); it != o.cend(); ++it) {
            UUID u(it.key());
            if (block->nets.count(u) > 0) {
                const auto &net = block->nets.at(u);
                if (net.is_port) {
                    try {
                        connections.emplace(std::make_pair(u, Connection(it.value(), bl)));
                    }
                    catch (const std::exception &e) {
                        Logger::log_critical("error loading connection to " + refdes + "." + net.name,
                                             Logger::Domain::BLOCK, e.what());
                    }
                }
                else {
                    Logger::log_critical("connection to non-port net at " + refdes + "." + net.name,
                                         Logger::Domain::BLOCK);
                }
            }
            else {
                Logger::log_critical("connection to nonexistent net at " + refdes, Logger::Domain::BLOCK);
            }
        }
    }
}
BlockInstance::BlockInstance(const UUID &uu, Block &b) : uuid(uu), block(&b)
{
}
UUID BlockInstance::get_uuid() const
{
    return uuid;
}

UUID BlockInstance::peek_block_uuid(const json &j)
{
    return j.at("block").get<std::string>();
}

std::string BlockInstance::replace_text(const std::string &t, bool *replaced) const
{
    if (replaced)
        *replaced = false;
    if (t == "$REFDES" || t == "$RD") {
        if (replaced)
            *replaced = true;
        return refdes;
    }
    else if (t == "$NAME") {
        if (replaced)
            *replaced = true;
        return block->name;
    }
    else {
        return t;
    }
}

json BlockInstance::serialize() const
{
    json j;
    j["refdes"] = refdes;
    j["block"] = (std::string)block->uuid;
    j["connections"] = json::object();
    for (const auto &it : connections) {
        j["connections"][(std::string)it.first] = it.second.serialize();
    }
    return j;
}

BlockInstanceMapping::ComponentInfo::ComponentInfo(const json &j)
    : refdes(j.at("refdes").get<std::string>()), nopopulate(j.at("nopopulate").get<bool>())
{
}

BlockInstanceMapping::ComponentInfo::ComponentInfo()
{
}

json BlockInstanceMapping::ComponentInfo::serialize() const
{
    json j;
    j["refdes"] = refdes;
    j["nopopulate"] = nopopulate;
    return j;
}


BlockInstanceMapping::BlockInstanceMapping(const Block &b) : block(b.uuid)
{
}

BlockInstanceMapping::BlockInstanceMapping(const json &j) : block(j.at("block").get<std::string>())
{
    for (const auto &[k, value] : j.at("components").items()) {
        const auto u = UUID(k);
        components.emplace(std::piecewise_construct, std::forward_as_tuple(u), std::forward_as_tuple(value));
    }
}

json BlockInstanceMapping::serialize() const
{
    json j;
    j["block"] = (std::string)block;
    j["components"] = json::object();
    for (const auto &it : components) {
        j["components"][(std::string)it.first] = it.second.serialize();
    }
    return j;
}


} // namespace horizon
