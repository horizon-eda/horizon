#include "board_rules.hpp"
#include "board.hpp"
#include "board_layers.hpp"
#include "util/polygon_arc_removal_proxy.hpp"
#include "util/geom_util.hpp"

namespace horizon {


namespace {
struct PackageOutline {
    ClipperLib::Paths top;
    ClipperLib::Paths bot;
};
} // namespace

RulesCheckResult BoardRules::check_height_restrictions(const class Board &b, RulesCheckCache &cache,
                                                       check_status_cb_t status_cb,
                                                       const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    if (r.check_disabled(rule_height_restrictions))
        return r;

    if (b.height_restrictions.size() == 0) {
        r.errors.emplace_back(RulesCheckErrorLevel::PASS, "no height restrictions");
        r.update();
        return r;
    }

    status_cb("collecting packages");
    std::map<UUID, PackageOutline> packages;
    for (const auto &[uu, pkg] : b.packages) {
        Placement tr = pkg.placement;
        if (pkg.flip)
            tr.invert_angle();

        const bool have_bottom_package =
                std::any_of(pkg.package.polygons.begin(), pkg.package.polygons.end(),
                            [](const auto &it) { return it.second.layer == BoardLayers::BOTTOM_PACKAGE; });

        auto &ol = packages[uu];

        for (const auto &[uu_poly, poly] : pkg.package.polygons) {
            if (poly.layer != BoardLayers::TOP_PACKAGE && poly.layer != BoardLayers::BOTTOM_PACKAGE)
                continue;

            PolygonArcRemovalProxy prx(poly);
            ClipperLib::Path path;
            path.reserve(poly.vertices.size());

            for (const auto &itv : prx.get().vertices) {
                auto p = tr.transform(itv.position);
                path.push_back({p.x, p.y});
            }

            if (poly.layer == BoardLayers::TOP_PACKAGE)
                ol.top.push_back(path);
            else if (poly.layer == BoardLayers::BOTTOM_PACKAGE)
                ol.bot.push_back(path);

            int64_t height_bot = 0;
            if (pkg.package.models.count(pkg.model))
                height_bot = pkg.package.models.at(pkg.model).height_bot;

            if (poly.layer == BoardLayers::TOP_PACKAGE && !have_bottom_package && height_bot)
                ol.bot.push_back(path);
        }

        if (ol.bot.size() == 0 && ol.top.size() == 0) {
            auto &err = r.errors.emplace_back(RulesCheckErrorLevel::WARN,
                                              "package " + pkg.component->refdes + " has no package outlines");
            err.has_location = true;
            err.location = pkg.placement.shift;
        }
    }

    const auto board_thickness = b.get_total_thickness();

    for (const auto &[uu_hr, hr] : b.height_restrictions) {
        auto &poly = *hr.polygon;
        if (poly.layer != BoardLayers::TOP_PACKAGE && poly.layer != BoardLayers::BOTTOM_PACKAGE) {
            auto &err = r.errors.emplace_back(RulesCheckErrorLevel::WARN,
                                              "ignored height restriction not in package layer");
            err.has_location = true;
            err.location = poly.vertices.front().position;
            continue;
        }
        ClipperLib::Path hr_path;
        {
            PolygonArcRemovalProxy prx(poly);
            for (const auto &itv : prx.get().vertices) {
                hr_path.push_back({itv.position.x, itv.position.y});
            }
        }

        for (const auto &[uu_pkg, pkg_outline] : packages) {
            const auto &paths = poly.layer == BoardLayers::TOP_PACKAGE ? pkg_outline.top : pkg_outline.bot;
            if (paths.empty())
                continue;
            const auto &pkg = b.packages.at(uu_pkg);
            const std::string layer_name = poly.layer == BoardLayers::TOP_PACKAGE ? "top" : "bottom";

            ClipperLib::Clipper clipper;
            clipper.AddPaths(paths, ClipperLib::ptClip, true);
            clipper.AddPath(hr_path, ClipperLib::ptSubject, true);
            ClipperLib::Paths isect;
            clipper.Execute(ClipperLib::ctIntersection, isect, ClipperLib::pftNonZero, ClipperLib::pftNonZero);
            if (!isect.empty()) { // overlap
                if (!pkg.package.models.count(pkg.model)) {
                    auto &err = r.errors.emplace_back(RulesCheckErrorLevel::WARN,
                                                      "package " + pkg.component->refdes + " has no 3D model");
                    err.has_location = true;
                    err.location = pkg.placement.shift;
                    err.layers = {poly.layer};
                    continue;
                }
                const auto model = pkg.package.models.at(pkg.model);
                const auto height = poly.layer == BoardLayers::TOP_PACKAGE
                                            ? (model.height_top - (pkg.flip ? board_thickness : 0))
                                            : (model.height_bot - (pkg.flip ? 0 : board_thickness));

                if (height == 0) {
                    auto &err = r.errors.emplace_back(RulesCheckErrorLevel::WARN, "package " + pkg.component->refdes
                                                                                          + " has no " + layer_name
                                                                                          + " height");
                    err.has_location = true;
                    err.location = pkg.placement.shift;
                    err.layers = {poly.layer};
                    continue;
                }
                if (height > hr.height) {
                    auto &err = r.errors.emplace_back(RulesCheckErrorLevel::FAIL,
                                                      "package " + pkg.component->refdes + " " + layer_name
                                                              + " height (" + dim_to_string_nlz(height, false)
                                                              + ") exceeds height restriction of "
                                                              + dim_to_string_nlz(hr.height, false));
                    err.has_location = true;
                    err.location = pkg.placement.shift;
                    err.error_polygons = isect;
                    err.layers = {poly.layer};
                }
            }
        }
    }


    r.update();
    return r;
}

} // namespace horizon
