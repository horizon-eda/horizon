#include "via_definition.hpp"
#include <nlohmann/json.hpp>
#include "common/common.hpp"

namespace horizon {
ViaDefinition::ViaDefinition(const UUID &uu) : uuid(uu)
{
    parameters[ParameterID::VIA_DIAMETER] = .5_mm;
    parameters[ParameterID::HOLE_DIAMETER] = .2_mm;
}

ViaDefinition::ViaDefinition(const UUID &uu, const json &j)
    : uuid(uu), name(j.at("name").get<std::string>()), padstack(j.at("padstack").get<std::string>()),
      parameters(parameter_set_from_json(j.at("parameters"))), span(j.at("span"))
{
}

json ViaDefinition::serialize() const
{
    json j;
    j["name"] = name;
    j["padstack"] = (std::string)padstack;
    j["parameters"] = parameter_set_serialize(parameters);
    j["span"] = span.serialize();
    return j;
}

} // namespace horizon
