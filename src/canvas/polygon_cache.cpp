#include "polygon_cache.hpp"
#include "common/polygon.hpp"
#include <glibmm/checksum.h>
#include "polypartition/polypartition.h"


namespace horizon {

namespace {
class PolygonHash {
public:
    static std::array<uint8_t, 20> hash(const Polygon &poly)
    {
        PolygonHash h;
        h.update(poly);
        std::array<uint8_t, 20> r;
        gsize sz = r.size();
        h.checksum.get_digest(r.data(), &sz);
        assert(sz == r.size());
        return r;
    }

private:
    PolygonHash() : checksum(Glib::Checksum::CHECKSUM_SHA1)
    {
    }
    void update(const class Padstack &padstack);

    Glib::Checksum checksum;

    void update(int64_t i)
    {
        checksum.update(reinterpret_cast<const guchar *>(&i), sizeof(i));
    }
    void update(const Coordi &c)
    {
        update(c.x);
        update(c.y);
    }

    void update(const class Polygon &p)
    {
        for (const auto &it : p.vertices) {
            update(it.position);
            update(static_cast<int>(it.type));
            if (it.type == Polygon::Vertex::Type::ARC)
                update(it.arc_center);
        }
    }
};
} // namespace


static const Coordf coordf_from_pt(const TPPLPoint &p)
{
    return Coordf(p.x, p.y);
}

const std::vector<std::array<Coordf, 3>> &PolygonCache::get_triangles(const Polygon &poly)
{
    const auto hash = PolygonHash::hash(poly);
    if (cache.count(hash))
        return cache.at(hash);


    TPPLPoly po;
    po.Init(poly.vertices.size());
    po.SetHole(false);
    {
        size_t i = 0;
        for (auto &it : poly.vertices) {
            po[i].x = it.position.x;
            po[i].y = it.position.y;
            i++;
        }
    }
    std::list<TPPLPoly> outpolys;
    TPPLPartition part;
    po.SetOrientation(TPPL_ORIENTATION_CCW);
    part.Triangulate_EC(&po, &outpolys);

    auto &tris = cache[hash];
    tris.reserve(outpolys.size());
    for (auto &tri : outpolys) {
        assert(tri.GetNumPoints() == 3);
        tris.emplace_back(
                std::array<Coordf, 3>{coordf_from_pt(tri[0]), coordf_from_pt(tri[1]), coordf_from_pt(tri[2])});
    }
    return tris;
}
} // namespace horizon
