#include "object_provider.hpp"
#include "arc.hpp"
#include "nlohmann/json.hpp"
#include "util/bbox_accumulator.hpp"
#include "util/geom_util.hpp"
#include <algorithm>

namespace horizon {

Arc::Arc(const UUID &uu, const json &j, ObjectProvider &obj)
    : uuid(uu), to(obj.get_junction(j.at("to").get<std::string>())),
      from(obj.get_junction(j.at("from").get<std::string>())),
      center(obj.get_junction(j.at("center").get<std::string>())), width(j.value("width", 0)),
      layer(j.value("layer", 0))
{
}

Arc::Arc(UUID uu) : uuid(uu)
{
}

void Arc::reverse()
{
    std::swap(to, from);
}

std::pair<Coordi, Coordi> Arc::get_bbox() const
{
    BBoxAccumulator<Coordi::type> acc;
    acc.accumulate(from->position);
    acc.accumulate(to->position);
    const auto c = project_onto_perp_bisector(from->position, to->position, center->position).to_coordi();
    const auto radius0 = (c - from->position).magd();
    const auto a0 = c2pi((from->position - c).angle());
    const auto a1 = c2pi((to->position - c).angle());
    float dphi = c2pi(a1 - a0);
    for (int i = 0; i < 4; i++) {
        const auto a = i * M_PI / 4;
        if (c2pi(a - a0) < dphi)
            acc.accumulate(c + Coordd::euler(radius0, a).to_coordi());
    }
    return acc.get();
}

json Arc::serialize() const
{
    json j;
    j["from"] = (std::string)from.uuid;
    j["to"] = (std::string)to.uuid;
    j["center"] = (std::string)center.uuid;
    j["width"] = width;
    j["layer"] = layer;
    return j;
}
} // namespace horizon
