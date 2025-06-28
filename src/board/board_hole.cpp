#include "board_hole.hpp"
#include "pool/ipool.hpp"
#include <nlohmann/json.hpp>
#include "block/block.hpp"

namespace horizon {

BoardHole::BoardHole(const UUID &uu, const json &j, Block *block, IPool &pool)
    : uuid(uu), pool_padstack(pool.get_padstack(j.at("padstack").get<std::string>())), padstack(*pool_padstack),
      placement(j.at("placement")), parameter_set(parameter_set_from_json(j.at("parameter_set")))
{
    if (j.count("net")) {
        if (block)
            net = &block->nets.at(j.at("net").get<std::string>());
        else
            net.uuid = j.at("net").get<std::string>();
    }
}
BoardHole::BoardHole(const UUID &uu, std::shared_ptr<const Padstack> ps) : uuid(uu), pool_padstack(ps), padstack(*ps)
{
}

json BoardHole::serialize() const
{
    json j;
    j["padstack"] = (std::string)pool_padstack->uuid;
    j["placement"] = placement.serialize();
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    if (net)
        j["net"] = net->uuid;
    return j;
}

UUID BoardHole::get_uuid() const
{
    return uuid;
}
} // namespace horizon
