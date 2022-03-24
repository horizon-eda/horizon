#include "board.hpp"
#include "board_layers.hpp"
#include "util/accumulator.hpp"
#include "rules/cache.hpp"
#include "common/patch_type_names.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"
#include "board_rules_check_util.hpp"
#include "util/polygon_arc_removal_proxy.hpp"

namespace horizon {
RulesCheckResult BoardRules::check_plane_priorities(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;


    std::map<int, std::vector<const Polygon *>> polys_by_layer;
    for (const auto &[uu, poly] : brd.polygons) {
        if (poly.usage && poly.usage->get_type() == PolygonUsage::Type::PLANE) {
            polys_by_layer[poly.layer].push_back(&poly);
        }
    }

    for (const auto &[layer, polys] : polys_by_layer) {
        struct Path {
            Path(ClipperLib::Path &&p, const Plane &pl) : path(p), plane(pl)
            {
            }
            ClipperLib::Path path;
            const Plane &plane;
        };
        std::vector<Path> paths;
        for (const auto ipoly : polys) {
            const PolygonArcRemovalProxy arc_removal_proxy{*ipoly, 64};
            const auto &poly = arc_removal_proxy.get();
            ClipperLib::Path path;
            path.reserve(poly.vertices.size());
            for (const auto &v : poly.vertices) {
                path.emplace_back(v.position.x, v.position.y);
            }
            paths.emplace_back(std::move(path), dynamic_cast<const Plane &>(*ipoly->usage));
        }
        const auto n = paths.size();
        for (size_t i = 0; i < n; i++) {
            for (size_t j = 0; j < n; j++) {
                if (i < j) {
                    const auto &path_a = paths.at(i);
                    const auto &path_b = paths.at(j);
                    ClipperLib::Paths isect;
                    {
                        ClipperLib::Clipper clipper;
                        clipper.AddPath(path_a.path, ClipperLib::ptClip, true);
                        clipper.AddPath(path_b.path, ClipperLib::ptSubject, true);
                        clipper.Execute(ClipperLib::ctIntersection, isect);
                    }
                    if (isect.size()) {
                        // planes do overlap, check if one plane completly encloses the other
                        bool a_encloses_b = false;
                        bool b_encloses_a = false;
                        for (const auto &[a, b, e] : {std::make_tuple(path_a, path_b, &a_encloses_b),
                                                      std::make_tuple(path_b, path_a, &b_encloses_a)}) {
                            ClipperLib::Paths remainder;

                            ClipperLib::Clipper clipper;
                            clipper.AddPath(a.path, ClipperLib::ptClip, true);
                            clipper.AddPath(b.path, ClipperLib::ptSubject, true);
                            clipper.Execute(ClipperLib::ctDifference, remainder);
                            if (remainder.size() == 0) { // a encloses b
                                *e = true;
                            }
                        }
                        if (a_encloses_b && b_encloses_a) {
                            // looks like both planes are identical
                            for (const auto &ite : isect) {
                                r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                                auto &e = r.errors.back();
                                e.has_location = true;
                                Accumulator<Coordi> acc;
                                for (const auto &ite2 : ite) {
                                    acc.accumulate({ite2.X, ite2.Y});
                                }
                                e.location = acc.get();
                                e.comment = "Plane outline " + get_net_name(path_a.plane.net)
                                            + " is identical to plane outline " + get_net_name(path_b.plane.net)
                                            + " on layer " + brd.get_layers().at(layer).name;
                                e.error_polygons = {ite};
                                e.layers.insert(layer);
                            }
                        }
                        else if (a_encloses_b || b_encloses_a) {
                            // enclosing plane needs to have lower priority (higher number) than enclosed plane
                            if ((a_encloses_b && (path_a.plane.priority <= path_b.plane.priority))
                                || (b_encloses_a && (path_b.plane.priority <= path_a.plane.priority))) {
                                const auto &enclosing_plane = (a_encloses_b ? path_a : path_b).plane;
                                const auto &enclosed_plane = (a_encloses_b ? path_b : path_a).plane;
                                for (const auto &ite : isect) {
                                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                                    auto &e = r.errors.back();
                                    e.has_location = true;
                                    Accumulator<Coordi> acc;
                                    for (const auto &ite2 : ite) {
                                        acc.accumulate({ite2.X, ite2.Y});
                                    }
                                    e.location = acc.get();
                                    e.comment = "Plane " + get_net_name(enclosing_plane.net) + " enclosing plane  "
                                                + get_net_name(enclosed_plane.net) + " on layer "
                                                + brd.get_layers().at(layer).name
                                                + " needs higher fill order than enclosed plane";
                                    e.error_polygons = {ite};
                                    e.layers.insert(layer);
                                }
                            }
                        }
                        else { // no polygon encloses the other, need to have different priorities
                            if (path_a.plane.priority == path_b.plane.priority) {
                                for (const auto &ite : isect) {
                                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                                    auto &e = r.errors.back();
                                    e.has_location = true;
                                    Accumulator<Coordi> acc;
                                    for (const auto &ite2 : ite) {
                                        acc.accumulate({ite2.X, ite2.Y});
                                    }
                                    e.location = acc.get();
                                    e.comment = "Plane " + get_net_name(path_a.plane.net) + " overlapping plane "
                                                + get_net_name(path_b.plane.net) + " of fill order "
                                                + std::to_string(path_a.plane.priority) + " on layer "
                                                + brd.get_layers().at(layer).name;
                                    e.error_polygons = {ite};
                                    e.layers.insert(layer);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    r.update();
    return r;
}

} // namespace horizon
