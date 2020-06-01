#include "picture.hpp"
#include "nlohmann/json.hpp"
#include <cairomm/cairomm.h>
#include <glib.h>

namespace horizon {
Picture::Picture(const UUID &uu) : uuid(uu)
{
}

Picture::Picture(const UUID &uu, const json &j)
    : uuid(uu), placement(j.at("placement")), px_size(j.at("px_size")), data_uuid(j.at("data").get<std::string>())
{
}

json Picture::serialize() const
{
    json j;
    j["uuid"] = (std::string)uuid;
    j["placement"] = placement.serialize();
    j["px_size"] = px_size;
    j["data"] = (std::string)data_uuid;
    return j;
}


} // namespace horizon
