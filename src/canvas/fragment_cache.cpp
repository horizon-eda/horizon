#include "fragment_cache.hpp"
#include "board/plane.hpp"
#include "poly2tri/poly2tri.h"

namespace horizon {

static const Coordf coordf_from_pt(const p2t::Point *pt)
{
    return Coordf(pt->x, pt->y);
}

const std::vector<std::array<Coordf, 3>> &FragmentCache::get_triangles(const Plane &plane)
{
    if (planes.count(plane.uuid) == 0
        || planes.at(plane.uuid).revision != plane.get_revision()) { // not found or out of date
        planes[plane.uuid];
        planes[plane.uuid].revision = plane.get_revision();
        planes[plane.uuid].triangles.clear();
        for (const auto &frag : plane.fragments) {
            std::vector<p2t::Point> point_store;
            size_t pts_total = 0;
            for (const auto &it : frag.paths)
                pts_total += it.size();
            point_store.reserve(pts_total); // important so that iterators won't
                                            // get invalidated

            std::vector<p2t::Point *> contour;
            contour.reserve(frag.paths.front().size());
            for (const auto &p : frag.paths.front()) {
                point_store.emplace_back(p.X, p.Y);
                contour.push_back(&point_store.back());
            }
            p2t::CDT cdt(contour);
            for (size_t i = 1; i < frag.paths.size(); i++) {
                auto &path = frag.paths.at(i);
                std::vector<p2t::Point *> hole;
                hole.reserve(path.size());
                for (const auto &p : path) {
                    point_store.emplace_back(p.X, p.Y);
                    hole.push_back(&point_store.back());
                }
                cdt.AddHole(hole);
            }
            cdt.Triangulate();
            auto tris = cdt.GetTriangles();
            auto &tris_out = planes[plane.uuid].triangles;
            for (const auto tri : tris) {
                tris_out.emplace_back();
                for (int i = 0; i < 3; i++) {
                    tris_out.back()[i] = coordf_from_pt(tri->GetPoint(i));
                }
            }
        }
    }

    return planes.at(plane.uuid).triangles;
}
} // namespace horizon
