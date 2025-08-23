#include "height_restriction.hpp"
#include "common/lut.hpp"
#include "nlohmann/json.hpp"
#include "common/object_provider.hpp"

namespace horizon {

HeightRestriction::HeightRestriction(const UUID &uu, const json &j, ObjectProvider &prv)
    : uuid(uu), polygon(prv.get_polygon(j.at("polygon").get<std::string>())), height(j.at("height").get<int64_t>())
{
}

HeightRestriction::HeightRestriction(const UUID &uu) : uuid(uu)
{
}

ObjectType HeightRestriction::get_type() const
{
    return ObjectType::HEIGHT_RESTRICTION;
}

UUID HeightRestriction::get_uuid() const
{
    return uuid;
}

json HeightRestriction::serialize() const
{
    json j;
    j["polygon"] = (std::string)polygon->uuid;
    j["height"] = height;
    return j;
}


} // namespace horizon
