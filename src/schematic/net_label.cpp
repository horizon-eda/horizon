#include "net_label.hpp"
#include "common/lut.hpp"
#include "sheet.hpp"
#include <nlohmann/json.hpp>

namespace horizon {
NetLabel::NetLabel(const UUID &uu, const json &j, Sheet *sheet)
    : uuid(uu), orientation(orientation_lut.lookup(j.at("orientation"))), size(j.value("size", 2500000)),
      offsheet_refs(j.value("offsheet_refs", true)), show_port(j.value("show_port", false))
{
    if (sheet)
        junction = &sheet->junctions.at(j.at("junction").get<std::string>());
    else
        junction.uuid = j.at("junction").get<std::string>();
    if (j.count("last_net"))
        last_net = j.at("last_net").get<std::string>();
}

NetLabel::NetLabel(const UUID &uu) : uuid(uu)
{
}

json NetLabel::serialize() const
{
    json j;
    j["junction"] = (std::string)junction->uuid;
    j["orientation"] = orientation_lut.lookup_reverse(orientation);
    j["size"] = size;
    j["offsheet_refs"] = offsheet_refs;
    j["show_port"] = show_port;
    if (last_net)
        j["last_net"] = (std::string)last_net;
    return j;
}
} // namespace horizon
