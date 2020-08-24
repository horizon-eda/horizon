#include "board_decal.hpp"
#include "nlohmann/json.hpp"
#include "pool/ipool.hpp"

namespace horizon {

BoardDecal::BoardDecal(const UUID &uu, const json &j, IPool &pool)
    : uuid(uu), pool_decal(pool.get_decal(j.at("decal").get<std::string>())), decal(*pool_decal),
      placement(j.at("placement")), flip(j.at("flip").get<bool>())
{
}

BoardDecal::BoardDecal(const UUID &uu) : uuid(uu), pool_decal(nullptr), decal(UUID())
{
}

json BoardDecal::serialize() const
{
    json j;
    j["decal"] = (std::string)pool_decal->uuid;
    j["placement"] = placement.serialize();
    j["flip"] = flip;
    return j;
}

UUID BoardDecal::get_uuid() const
{
    return uuid;
}

static Coordi operator*(const Coordi &c, double m)
{
    return Coordi(c.x * m, c.y * m);
}

void BoardDecal::apply_scale()
{
    for (auto &[uu, it] : decal.junctions) {
        it.position = pool_decal->junctions.at(uu).position * scale;
    }
    for (auto &[uu, it] : decal.polygons) {
        const auto &p = pool_decal->polygons.at(uu);
        for (size_t i = 0; i < it.vertices.size(); i++) {
            it.vertices.at(i).position = p.vertices.at(i).position * scale;
            it.vertices.at(i).arc_center = p.vertices.at(i).arc_center * scale;
        }
    }
    for (auto &[uu, it] : decal.texts) {
        const auto &t = pool_decal->texts.at(uu);
        it.size = t.size * scale;
        it.width = t.width * scale;
        it.placement.shift = t.placement.shift * scale;
    }
    for (auto &[uu, it] : decal.lines) {
        it.width = pool_decal->lines.at(uu).width * scale;
    }
    for (auto &[uu, it] : decal.arcs) {
        it.width = pool_decal->lines.at(uu).width * scale;
    }
}

} // namespace horizon
