#include "picture.hpp"
#include "nlohmann/json.hpp"
#include <cairomm/cairomm.h>
#include <glib.h>

namespace horizon {
Picture::Picture(const UUID &uu) : uuid(uu)
{
}

Picture::Picture(const UUID &uu, const json &j)
    : uuid(uu), placement(j.at("placement")), on_top(j.value("on_top", false)), opacity(j.value("opacity", 1.0)),
      show_border(j.value("show_border", false)), px_size(j.at("px_size")), data_uuid(j.at("data").get<std::string>())
{
}

json Picture::serialize() const
{
    json j;
    j["uuid"] = (std::string)uuid;
    j["placement"] = placement.serialize();
    j["on_top"] = on_top;
    j["opacity"] = opacity;
    if (show_border)
        j["show_border"] = true;
    j["px_size"] = px_size;
    j["data"] = (std::string)data_uuid;
    return j;
}

bool Picture::get_show_border() const
{
    return show_border || (opacity < min_opacity);
}

} // namespace horizon
