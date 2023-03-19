#include "board.hpp"
#include "board_layers.hpp"
#include "board_rules.hpp"
#include "common/patch_type_names.hpp"
#include "rules/cache.hpp"
#include "util/accumulator.hpp"
#include "util/geom_util.hpp"
#include "util/polygon_arc_removal_proxy.hpp"
#include <thread>
#include <future>
#include <sstream>
#include "util/named_vector.hpp"
#include "util/util.hpp"
#include "board_rules_check_util.hpp"
#include <iostream>

namespace horizon {

RulesCheckResult BoardRules::check_track_width(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto rules = get_rules_sorted<RuleTrackWidth>();
    for (const auto &it : brd.tracks) {
        auto width = it.second.width;
        Net *net = it.second.net;
        auto layer = it.second.layer;
        auto &track = it.second;
        for (auto ru : rules) {
            if (ru->enabled && ru->match.match(net)) {
                if (ru->widths.count(layer)) {
                    const auto &ws = ru->widths.at(layer);
                    if (width < ws.min || width > ws.max) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        e.layers.insert(layer);
                        e.location = (track.from.get_position() + track.to.get_position()) / 2;
                        e.comment = "Track width " + dim_to_string(width);
                        if (width < ws.min) {
                            e.comment += " is less than " + dim_to_string(ws.min);
                        }
                        else {
                            e.comment += " is greater than " + dim_to_string(ws.max);
                        }
                    }
                }
                break;
            }
        }
    }
    r.update();
    return r;
}

static RulesCheckError *check_hole(RulesCheckResult &r, uint64_t dia, const RuleHoleSize *ru, const std::string &what)
{
    if (dia < ru->diameter_min || dia > ru->diameter_max) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &e = r.errors.back();
        e.has_location = true;
        e.comment = what + " diameter " + dim_to_string(dia);
        if (dia < ru->diameter_min) {
            e.comment += " is less than " + dim_to_string(ru->diameter_min);
        }
        else {
            e.comment += " is greater than " + dim_to_string(ru->diameter_max);
        }
        return &e;
    }
    return nullptr;
}

RulesCheckResult BoardRules::check_hole_size(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto rules = get_rules_sorted<RuleHoleSize>();
    for (const auto &it : brd.holes) {
        Net *net = it.second.net;
        for (const auto &it_hole : it.second.padstack.holes) {
            auto dia = it_hole.second.diameter;
            for (auto ru : rules) {
                if (ru->enabled && ru->match.match(net)) {
                    if (auto e = check_hole(r, dia, ru, "Hole")) {
                        e->location = it.second.placement.shift;
                    }
                    break;
                }
            }
        }
    }

    for (const auto &it : brd.vias) {
        Net *net = it.second.junction->net;
        for (const auto &it_hole : it.second.padstack.holes) {
            auto dia = it_hole.second.diameter;
            for (auto ru : rules) {
                if (ru->enabled && ru->match.match(net)) {
                    if (auto e = check_hole(r, dia, ru, "Via")) {
                        e->location = it.second.junction->position;
                    }
                    break;
                }
            }
        }
    }

    for (const auto &it : brd.packages) {
        for (const auto &it_pad : it.second.package.pads) {
            Net *net = it_pad.second.net;
            for (const auto &it_hole : it_pad.second.padstack.holes) {
                auto dia = it_hole.second.diameter;
                for (auto ru : rules) {
                    if (ru->enabled && ru->match.match(net)) {
                        if (auto e = check_hole(r, dia, ru, "Pad hole")) {
                            auto p = it.second.placement;
                            if (it.second.flip) {
                                p.invert_angle();
                            }
                            e->location =
                                    p.transform(it_pad.second.placement.transform(it_hole.second.placement.shift));
                        }
                        break;
                    }
                }
            }
        }
    }
    r.update();
    return r;
}

RulesCheckResult BoardRules::check_clearance_silkscreen_exposed_copper(const Board &brd, RulesCheckCache &cache,
                                                                       check_status_cb_t status_cb,
                                                                       const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
    const auto &patches = c.get_canvas().get_patches();

    const auto &rule = rule_clearance_silkscreen_exposed_copper;

    if (r.check_disabled(rule))
        return r;

    if (r.check_cancelled(cancel))
        return r;

    struct { // Store layers to avoid redundancies for top and bottom side
        BoardLayers::Layer copper;
        BoardLayers::Layer mask;
        BoardLayers::Layer silkscreen;
        uint64_t clearance;
        bool top;
    } top_side, bottom_side;

    top_side.copper = BoardLayers::TOP_COPPER;
    top_side.mask = BoardLayers::TOP_MASK;
    top_side.silkscreen = BoardLayers::TOP_SILKSCREEN;
    top_side.clearance = rule.clearance_top;
    top_side.top = true;

    bottom_side.copper = BoardLayers::BOTTOM_COPPER;
    bottom_side.mask = BoardLayers::BOTTOM_MASK;
    bottom_side.silkscreen = BoardLayers::BOTTOM_SILKSCREEN;
    bottom_side.clearance = rule.clearance_top;
    bottom_side.top = false;

    for (const auto &side : {top_side, bottom_side}) {
        ClipperLib::Clipper copper_outline;
        // Generate exposed copper
        for (const auto &[key, paths] : patches) {
            if (key.layer == side.copper
                && (!rule.pads_only || key.type == PatchType::PAD || key.type == PatchType::PAD_TH)) {
                copper_outline.AddPaths(paths, ClipperLib::ptSubject, true);
            }
            else if (key.layer == side.mask) {
                copper_outline.AddPaths(paths, ClipperLib::ptClip, true);
            }
        }
        ClipperLib::Paths exposed_copper;
        copper_outline.Execute(ClipperLib::ctIntersection, exposed_copper, ClipperLib::pftNonZero);

        if (r.check_cancelled(cancel))
            return r;

        // Expand exposed copper as this should be faster then expanding silkscreen
        ClipperLib::ClipperOffset ofs;
        ofs.ArcTolerance = 10e3;
        ofs.AddPaths(exposed_copper, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
        ClipperLib::Paths exposed_copper_expanded;
        ofs.Execute(exposed_copper_expanded, side.clearance);

        // Check everything on the silkscreen for overlap with the expanded exposed copper
        for (const auto &[key, paths] : patches) {
            if (r.check_cancelled(cancel))
                return r;

            if (key.layer == side.silkscreen) {
                ClipperLib::Clipper clipper;
                clipper.AddPaths(exposed_copper_expanded, ClipperLib::ptClip, true);
                clipper.AddPaths(paths, ClipperLib::ptSubject, true);
                ClipperLib::Paths errors;
                clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // no intersection: no clearance violation
                if (errors.size() > 0) {
                    for (const auto &ite : errors) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        Accumulator<Coordi> acc;
                        for (const auto &ite2 : ite) {
                            acc.accumulate({ite2.X, ite2.Y});
                        }
                        e.location = acc.get();
                        if (side.top) {
                            e.comment = "Top side: Silkscreen near exposed copper";
                        }
                        else {
                            e.comment = "Bottom side: Silkscreen near exposed copper";
                        }
                        e.error_polygons = {ite};
                        if (side.top)
                            e.layers.insert(BoardLayers::TOP_SILKSCREEN);
                        else
                            e.layers.insert(BoardLayers::BOTTOM_SILKSCREEN);
                    }
                }
            }
        }
    }
    r.update();

    return r;
}

RulesCheckResult BoardRules::check_preflight(const Board &brd) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    if (!rule_preflight_checks.enabled) {
        r.level = RulesCheckErrorLevel::DISABLED;
        return r;
    }

    for (const auto &it_net : brd.airwires) {
        const auto &net = brd.block->nets.at(it_net.first);
        for (const auto &it : it_net.second) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            e.location = (it.from.get_position() + it.to.get_position()) / 2;
            std::string prefix = "Airwire";
            if (it.from.get_position() == it.to.get_position()) {
                prefix = "Zero length airwire";
            }
            e.comment = prefix + " of net " + brd.block->get_net_name(net.uuid);
        }
    }
    for (const auto &it : brd.planes) {
        if (it.second.fragments.size() == 0) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            e.location = it.second.polygon->vertices.front().position;
            e.comment = "Plane of net " + (it.second.net ? it.second.net->name : "No net") + " on layer "
                        + brd.get_layers().at(it.second.polygon->layer).name + " has no fragments";
        }
    }
    for (const auto &it : brd.block->components) {
        if (!it.second.part) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = false;
            e.comment = "Component " + it.second.refdes + " has no part";
        }
    }
    std::set<const Component *> unplaced_components;
    std::transform(brd.block->components.begin(), brd.block->components.end(),
                   std::inserter(unplaced_components, unplaced_components.begin()),
                   [](const auto &it) { return &it.second; });

    for (const auto &it : brd.packages) {
        unplaced_components.erase(it.second.component);
    }

    for (const auto &it : unplaced_components) {
        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
        auto &e = r.errors.back();
        e.has_location = false;
        e.comment = "Component " + it->refdes + " is not placed";
    }

    for (const auto &it : brd.tracks) {
        auto &track = it.second;
        if (!track.net) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            e.location = (track.from.get_position() + track.to.get_position()) / 2;
            e.comment = "Track on layer " + brd.get_layers().at(track.layer).name + " has no net";
        }
    }

    for (const auto &it : brd.polygons) {
        bool is_keepout = dynamic_cast<const Keepout *>(it.second.usage.ptr);
        bool is_plane = dynamic_cast<Plane *>(it.second.usage.ptr);
        if (BoardLayers::is_copper(it.second.layer) && !(is_plane || is_keepout)) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = it.second.vertices.size();
            if (e.has_location)
                e.location = it.second.vertices.front().position;
            e.comment =
                    "Polygon on layer " + brd.get_layers().at(it.second.layer).name + " is not a keepout or a plane";
        }
    }

    for (const auto &it : brd.lines) {
        if (it.second.layer == BoardLayers::L_OUTLINE) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            if (e.has_location)
                e.location = it.second.from->position;
            e.comment = "Line on outline layer. Use polygons to define board outline.";
        }
    }

    for (const auto &it : brd.arcs) {
        if (it.second.layer == BoardLayers::L_OUTLINE) {
            r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
            auto &e = r.errors.back();
            e.has_location = true;
            if (e.has_location)
                e.location = it.second.from->position;
            e.comment = "Arc on outline layer. Use polygons to define board outline.";
        }
    }

    {
        auto outline_errors = brd.get_outline_and_errors();
        for (const auto &e : outline_errors.errors.errors) {
            r.errors.emplace_back(e);
        }
    }

    r.update();
    return r;
}


RulesCheckResult BoardRules::check_clearance_copper_keepout(const Board &brd, RulesCheckCache &cache,
                                                            check_status_cb_t status_cb,
                                                            const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    status_cb("Getting patches");
    auto rules = get_rules_sorted<RuleClearanceCopperKeepout>();
    auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
    const auto &patches = c.get_canvas().get_patches();
    auto keepout_contours = brd.get_keepout_contours();
    auto n_patches = patches.size();
    auto n_keepouts = keepout_contours.size();
    int i_keepout = 0;
    status_cb("Intersecting");
    for (const auto &it_keepout : keepout_contours) {
        i_keepout++;
        const auto keepout = it_keepout.keepout;

        int i_patch = 0;
        for (const auto &it : patches) {
            if (r.check_cancelled(cancel))
                return r;
            i_patch++;
            {
                std::stringstream ss;
                ss << "Keepout " << i_keepout << "/" << n_keepouts << ", patch " << i_patch << "/" << n_patches;
                status_cb(ss.str());
            }
            if (BoardLayers::is_copper(it.first.layer)
                && (it.first.layer.overlaps(keepout->polygon->layer) || keepout->all_cu_layers)
                && keepout->patch_types_cu.count(it.first.type)) {

                const Net *net = it.first.net ? &brd.block->nets.at(it.first.net) : nullptr;
                int64_t clearance = 0;
                for (const auto &rule : rules) {
                    if (rule->match.match(net)
                        && (rule->match_keepout.mode == RuleMatchKeepout::Mode::ALL
                            || (rule->match_keepout.mode == RuleMatchKeepout::Mode::KEEPOUT_CLASS
                                && rule->match_keepout.keepout_class == keepout->keepout_class))) {
                        clearance = rule->get_clearance(it.first.type);
                        break;
                    }
                }


                ClipperLib::Clipper clipper;
                if (clearance == 0) {
                    clipper.AddPath(it_keepout.contour, ClipperLib::ptClip, true);
                }
                else {
                    ClipperLib::Paths keepout_contour_expanded;
                    ClipperLib::ClipperOffset ofs;
                    ofs.ArcTolerance = 10e3;
                    ofs.AddPath(it_keepout.contour, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                    ofs.Execute(keepout_contour_expanded, clearance);
                    clipper.AddPaths(keepout_contour_expanded, ClipperLib::ptClip, true);
                }
                clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
                ClipperLib::Paths errors;
                clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

                // no intersection: no clearance violation
                if (errors.size() > 0) {
                    for (const auto &ite : errors) {
                        r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                        auto &e = r.errors.back();
                        e.has_location = true;
                        Accumulator<Coordi> acc;
                        for (const auto &ite2 : ite) {
                            acc.accumulate({ite2.X, ite2.Y});
                        }
                        e.location = acc.get();
                        e.comment = patch_type_names.at(it.first.type) + "(" + (net ? net->name : "") + ") on layer"
                                    + brd.get_layer_name(it.first.layer) + " near keepout";
                        e.error_polygons = {ite};
                        e.add_layer_range(brd, it.first.layer);
                    }
                }
            }
        }
    }
    r.update();
    return r;
}

RulesCheckResult BoardRules::check_clearance_same_net(const Board &brd, RulesCheckCache &cache,
                                                      check_status_cb_t status_cb, const std::atomic_bool &cancel) const
{
    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    status_cb("Getting patches");
    auto rules = get_rules_sorted<RuleClearanceSameNet>();
    auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
    const auto &patches = c.get_canvas().get_patches();

    if (r.check_cancelled(cancel))
        return r;
    status_cb("Building patch pairs");

    static const std::set<PatchType> patch_types = {PatchType::PAD, PatchType::PAD_TH, PatchType::VIA,
                                                    PatchType::HOLE_NPTH};
    std::set<std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey>> patch_pairs;


    // assemble a list of patch pairs we'll need to check
    for (const auto &it : patches) {
        for (const auto &it_other : patches) {
            if (it.first.layer.overlaps(it_other.first.layer) && patch_types.count(it.first.type)
                && patch_types.count(it_other.first.type) && it.first.type != it_other.first.type
                && it.first.net == it_other.first.net) {
                std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey> k = {it.first, it_other.first};
                auto k2 = k;
                std::swap(k2.first, k2.second);
                if (patch_pairs.count(k) == 0 && patch_pairs.count(k2) == 0) {
                    patch_pairs.emplace(k);
                }
            }
        }
        if (r.check_cancelled(cancel))
            return r;
    }

    for (const auto &[p1, p2] : patch_pairs) {
        if (r.check_cancelled(cancel))
            return r;

        const Net *net = p1.net ? &brd.block->nets.at(p1.net) : nullptr;

        const auto layer_isect = p1.layer.intersection(p2.layer).value();

        auto clearance =
                find_clearance(*this, &BoardRules::get_clearance_same_net, brd.get_layers_for_range(layer_isect),
                               std::forward_as_tuple(net), std::forward_as_tuple(p1.type, p2.type));

        if (clearance >= 0) {
            // expand p1 patch by clearance
            ClipperLib::ClipperOffset ofs;
            ofs.ArcTolerance = 10e3;
            ofs.AddPaths(patches.at(p1), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            ClipperLib::Paths paths_ofs;
            ofs.Execute(paths_ofs, clearance);

            // intersect expanded and this patch
            ClipperLib::Clipper clipper;
            clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
            clipper.AddPaths(patches.at(p2), ClipperLib::ptSubject, true);
            ClipperLib::Paths errors;
            clipper.Execute(ClipperLib::ctIntersection, errors, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

            // no intersection: no clearance violation
            if (errors.size() > 0) {
                for (const auto &ite : errors) {
                    r.errors.emplace_back(RulesCheckErrorLevel::FAIL);
                    auto &e = r.errors.back();
                    e.has_location = true;
                    Accumulator<Coordi> acc;
                    for (const auto &ite2 : ite) {
                        acc.accumulate({ite2.X, ite2.Y});
                    }
                    e.location = acc.get();
                    e.comment = patch_type_names.at(p1.type) + " near " + patch_type_names.at(p2.type) + " "
                                + get_net_name(net) + " on layer " + brd.get_layer_name(layer_isect);
                    e.error_polygons = {ite};
                    e.add_layer_range(brd, layer_isect);
                }
            }
        }
    }


    r.update();
    return r;
}

RulesCheckResult BoardRules::check(RuleID id, const Board &brd, RulesCheckCache &cache, check_status_cb_t status_cb,
                                   const std::atomic_bool &cancel) const
{
    switch (id) {
    case RuleID::TRACK_WIDTH:
        return BoardRules::check_track_width(brd);

    case RuleID::HOLE_SIZE:
        return BoardRules::check_hole_size(brd);

    case RuleID::CLEARANCE_COPPER:
        return BoardRules::check_clearance_copper(brd, cache, status_cb, cancel);

    case RuleID::CLEARANCE_COPPER_OTHER:
        return BoardRules::check_clearance_copper_non_copper(brd, cache, status_cb, cancel);

    case RuleID::CLEARANCE_SILKSCREEN_EXPOSED_COPPER:
        return BoardRules::check_clearance_silkscreen_exposed_copper(brd, cache, status_cb, cancel);

    case RuleID::CLEARANCE_COPPER_KEEPOUT:
        return BoardRules::check_clearance_copper_keepout(brd, cache, status_cb, cancel);

    case RuleID::PREFLIGHT_CHECKS:
        return BoardRules::check_preflight(brd);

    case RuleID::CLEARANCE_SAME_NET:
        return BoardRules::check_clearance_same_net(brd, cache, status_cb, cancel);

    case RuleID::PLANE:
        return BoardRules::check_plane_priorities(brd);

    case RuleID::NET_TIES:
        return BoardRules::check_net_ties(brd, cache, status_cb, cancel);

    case RuleID::BOARD_CONNECTIVITY:
        return BoardRules::check_board_connectivity(brd, cache, status_cb, cancel);

    default:
        return RulesCheckResult();
    }
}
} // namespace horizon
