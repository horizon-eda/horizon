#include "pad.hpp"
#include "pool/ipool.hpp"
#include "nlohmann/json.hpp"

namespace horizon {

Pad::Pad(const UUID &uu, const json &j, IPool &pool)
    : uuid(uu), pool_padstack(pool.get_padstack(j.at("padstack").get<std::string>())), padstack(*pool_padstack),
      placement(j.at("placement")), name(j.at("name").get<std::string>())
{
    if (j.count("parameter_set")) {
        parameter_set = parameter_set_from_json(j.at("parameter_set"));
    }
    if (j.count("parameters_fixed")) {
        const json &o = j["parameters_fixed"];
        for (const auto &value : o) {
            parameters_fixed.insert(parameter_id_from_string(value.get<std::string>()));
        }
    }
}
Pad::Pad(const UUID &uu, std::shared_ptr<const Padstack> ps) : uuid(uu), pool_padstack(ps), padstack(*ps)
{
}

json Pad::serialize() const
{
    json j;
    j["padstack"] = (std::string)pool_padstack->uuid;
    j["placement"] = placement.serialize();
    j["name"] = name;
    j["parameter_set"] = parameter_set_serialize(parameter_set);
    for (const auto &it : parameters_fixed) {
        j["parameters_fixed"].push_back(parameter_id_to_string(it));
    }

    return j;
}

UUID Pad::get_uuid() const
{
    return uuid;
}
} // namespace horizon
