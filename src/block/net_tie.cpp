#include "net_tie.hpp"
#include "block.hpp"
#include "nlohmann/json.hpp"

namespace horizon {
NetTie::NetTie(const UUID &uu) : uuid(uu)
{
}

NetTie::NetTie(const UUID &uu, const json &j, class Block &block)
    : uuid(uu), net_primary(&block.nets.at(j.at("net_primary").get<std::string>())),
      net_secondary(&block.nets.at(j.at("net_secondary").get<std::string>()))
{
}

json NetTie::serialize() const
{
    json j;
    j["net_primary"] = static_cast<std::string>(net_primary->uuid);
    j["net_secondary"] = static_cast<std::string>(net_secondary->uuid);
    return j;
}

void NetTie::update_refs(Block &block)
{
    net_primary.update(block.nets);
    net_secondary.update(block.nets);
}

UUID NetTie::get_uuid() const
{
    return uuid;
}

} // namespace horizon
