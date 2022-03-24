#include "board.hpp"
#include "board_layers.hpp"
#include "util/accumulator.hpp"
#include "rules/cache.hpp"
#include "common/patch_type_names.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"

namespace horizon {
class CanvasNetTies : public Canvas {
public:
    std::map<UUID, ClipperLib::Path> net_ties;

    CanvasNetTies();
    void push() override
    {
    }
    void request_push() override;

private:
    void img_polygon(const class Polygon &poly, bool tr) override;
};

CanvasNetTies::CanvasNetTies() : Canvas::Canvas()
{
    img_mode = true;
}
void CanvasNetTies::request_push()
{
}

void CanvasNetTies::img_polygon(const Polygon &ipoly, bool tr)
{
    auto poly = ipoly.remove_arcs(64);
    if (!BoardLayers::is_copper(poly.layer))
        return;
    if (object_refs_current.size() == 1 && object_refs_current.back().type == ObjectType::BOARD_NET_TIE) {
        auto uuid = object_refs_current.back().uuid;
        assert(!net_ties.count(uuid));
        ClipperLib::Path &t = net_ties[uuid];
        t.reserve(poly.vertices.size());
        if (tr) {
            for (const auto &it : poly.vertices) {
                auto p = transform.transform(it.position);
                t.emplace_back(p.x, p.y);
            }
        }
        else {
            for (const auto &it : poly.vertices) {
                t.emplace_back(it.position.x, it.position.y);
            }
        }
        if (ClipperLib::Orientation(t)) {
            std::reverse(t.begin(), t.end());
        }
    }
}


RulesCheckResult BoardRules::check_net_ties(const Board &brd, RulesCheckCache &cache, check_status_cb_t status_cb,
                                            const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    status_cb("Getting patches");
    auto rules = get_rules_sorted<RuleClearanceSameNet>();
    auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
    if (r.check_cancelled(cancel))
        return r;

    status_cb("Getting net ties");
    CanvasNetTies canvas_net_ties;
    canvas_net_ties.update(brd);
    if (r.check_cancelled(cancel))
        return r;

    {
        std::set<const NetTie *> net_ties_from_block;
        for (const auto &[uu, tie] : brd.block->net_ties) {
            net_ties_from_block.insert(&tie);
        }
        for (const auto &[uu, tie] : brd.net_ties) {
            net_ties_from_block.erase(tie.net_tie);
        }
        for (const auto tie : net_ties_from_block) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = false;
            e.comment = "Net tie for nets " + tie->net_primary->name + " and " + tie->net_secondary->name
                        + " not present on board";
        }
    }

    size_t i_tie = 1;
    for (const auto &[uu, path] : canvas_net_ties.net_ties) {
        const auto &tie = brd.net_ties.at(uu);
        status_cb("Checking net tie " + format_m_of_n(i_tie++, canvas_net_ties.net_ties.size()));
        // check that net tie connects its nets

        for (auto net : {tie.net_tie->net_primary, tie.net_tie->net_secondary}) {
            ClipperLib::Clipper clipper;
            clipper.AddPath(path, ClipperLib::ptSubject, true);
            for (const auto &[key, patch] : c.get_canvas().get_patches()) {
                if (key.layer == tie.layer && key.net == net->uuid && key.type != PatchType::NET_TIE)
                    clipper.AddPaths(patch, ClipperLib::ptClip, true);
            }

            ClipperLib::Paths result;
            clipper.Execute(ClipperLib::ctIntersection, result, ClipperLib::pftNonZero);
            if (result.empty()) {
                r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                auto &e = r.errors.back();
                e.has_location = true;
                e.location = (tie.from->position + tie.to->position) / 2;
                e.layers.insert(tie.layer);
                e.comment = "Net tie for nets " + tie.net_tie->net_primary->name + " and "
                            + tie.net_tie->net_secondary->name + " does not connect net " + net->name;
            }
            else {
                const auto multi = result.size() > 1;
                size_t i = 0;
                for (const auto &isect : result) {
                    i++;
                    if (multi) {
                        r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        e.location = Coordi(isect.front().X, isect.front().Y);
                        e.error_polygons = {isect};
                        e.layers.insert(tie.layer);
                        e.comment = "Net tie for nets " + tie.net_tie->net_primary->name + " and "
                                    + tie.net_tie->net_secondary->name + " connects net " + net->name
                                    + " more than once (" + format_m_of_n(i, result.size()) + ")";
                    }
                    const auto area = std::abs(ClipperLib::Area(isect));
                    const auto radius = tie.width / 2;
                    const auto min_area = (M_PI * radius * radius) * .99;
                    if (area < min_area) {
                        r.errors.emplace_back(RulesCheckErrorLevel::WARN);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        e.location = Coordi(isect.front().X, isect.front().Y);
                        e.error_polygons = {isect};
                        e.layers.insert(tie.layer);
                        e.comment = "Net tie for nets " + tie.net_tie->net_primary->name + " and "
                                    + tie.net_tie->net_secondary->name + " connects net " + net->name
                                    + " with insufficient area";
                    }
                }
            }
        }
    }

    r.update();
    return r;
}
} // namespace horizon
