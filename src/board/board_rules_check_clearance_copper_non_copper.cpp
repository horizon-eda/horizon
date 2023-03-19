#include "board.hpp"
#include "board_layers.hpp"
#include "util/accumulator.hpp"
#include "rules/cache.hpp"
#include "common/patch_type_names.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"
#include "board_rules_check_util.hpp"

namespace horizon {

RulesCheckResult BoardRules::check_clearance_copper_non_copper(const Board &brd, RulesCheckCache &cache,
                                                               check_status_cb_t status_cb,
                                                               const std::atomic_bool &cancel) const
{


    RulesCheckResult r;
    r.level = RulesCheckErrorLevel::PASS;
    auto &c = cache.get_cache<RulesCheckCacheBoardImage>();
    const auto &patches = c.get_canvas().get_patches();


    for (const auto &[key_npth, patch_npth] : patches) {
        if (key_npth.type != PatchType::HOLE_NPTH)
            continue;

        for (const auto &it : patches) {
            if (it.first.type != PatchType::HOLE_NPTH && BoardLayers::is_copper(it.first.layer)
                && key_npth.layer.overlaps(it.first.layer)) {
                if (r.check_cancelled(cancel))
                    return r;

                Net *net = it.first.net ? &brd.block->nets.at(it.first.net) : nullptr;

                const auto layer_isect = key_npth.layer.intersection(it.first.layer).value();

                const auto clearance = find_clearance(*this, &BoardRules::get_clearance_copper_other,
                                                      brd.get_layers_for_range(layer_isect), std::forward_as_tuple(net),
                                                      std::forward_as_tuple(it.first.type, PatchType::HOLE_NPTH));

                // expand npth patch by clearance
                ClipperLib::ClipperOffset ofs;
                ofs.ArcTolerance = 10e3;
                ofs.AddPaths(patch_npth, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                ClipperLib::Paths paths_ofs;
                ofs.Execute(paths_ofs, clearance);

                // intersect expanded and this patch
                ClipperLib::Clipper clipper;
                clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
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
                        e.comment = patch_type_names.at(it.first.type) + "(" + (net ? net->name : "") + ") on layer "
                                    + brd.get_layer_name(it.first.layer) + " near NPTH hole";
                        e.error_polygons = {ite};
                        e.add_layer_range(brd, it.first.layer);
                    }
                }
            }
        }
        r.update();
    }

    auto is_other = [](PatchType pt) { return pt == PatchType::OTHER || pt == PatchType::TEXT; };

    // assemble a list of patch pairs we'll need to check
    std::set<std::pair<CanvasPatch::PatchKey, CanvasPatch::PatchKey>> patch_pairs;
    for (const auto &it : patches) {
        for (const auto &it_other : patches) {
            if (it.first.layer.overlaps(it_other.first.layer)
                && ((is_other(it.first.type) ^ is_other(it_other.first.type)))) { // check non-other
                                                                                  // against other
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

    for (const auto &it : patch_pairs) {
        if (r.check_cancelled(cancel))
            return r;
        auto p_other = it.first;
        auto p_non_other = it.second;
        if (!is_other(p_other.type))
            std::swap(p_other, p_non_other);
        Net *net = p_non_other.net ? &brd.block->nets.at(p_non_other.net) : nullptr;

        // figure out the clearance between this patch pair
        const auto layer_isect = p_other.layer.intersection(p_non_other.layer).value();

        const auto clearance =
                find_clearance(*this, &BoardRules::get_clearance_copper_other, brd.get_layers_for_range(layer_isect),
                               std::forward_as_tuple(net), std::forward_as_tuple(p_non_other.type, p_other.type));

        // expand one of them by the clearance
        ClipperLib::ClipperOffset ofs;
        ofs.ArcTolerance = 10e3;
        ofs.AddPaths(patches.at(p_non_other), ClipperLib::jtRound, ClipperLib::etClosedPolygon);
        ClipperLib::Paths paths_ofs;
        ofs.Execute(paths_ofs, clearance);

        // intersect expanded and other patches
        ClipperLib::Clipper clipper;
        clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
        clipper.AddPaths(patches.at(p_other), ClipperLib::ptSubject, true);
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
                e.comment = patch_type_names.at(p_non_other.type) + "(" + (net ? net->name : "") + ") near "
                            + patch_type_names.at(p_other.type) + " on layer " + brd.get_layer_name(layer_isect);
                e.error_polygons = {ite};
                e.add_layer_range(brd, layer_isect);
            }
        }
    }

    r.update();

    CanvasPatch::PatchKey outline_key;
    outline_key.layer = BoardLayers::L_OUTLINE;
    outline_key.net = UUID();
    outline_key.type = PatchType::OTHER;
    if (patches.count(outline_key) == 0) {
        return r;
    }
    auto &patch_outline = patches.at(outline_key);

    // cleanup board outline so that it conforms to nonzero filling rule
    ClipperLib::Paths board_outline;
    {
        ClipperLib::Clipper cl_outline;
        cl_outline.AddPaths(patch_outline, ClipperLib::ptSubject, true);
        cl_outline.Execute(ClipperLib::ctUnion, board_outline, ClipperLib::pftEvenOdd);
    }

    for (const auto &it : patches) {
        if (BoardLayers::is_copper(it.first.layer)) { // need to check copper layer
            if (r.check_cancelled(cancel))
                return r;

            Net *net = it.first.net ? &brd.block->nets.at(it.first.net) : nullptr;

            const auto clearance = find_clearance(*this, &BoardRules::get_clearance_copper_other,
                                                  brd.get_layers_for_range(it.first.layer), std::forward_as_tuple(net),
                                                  std::forward_as_tuple(it.first.type, PatchType::BOARD_EDGE));

            // contract board outline patch by clearance
            ClipperLib::ClipperOffset ofs;
            ofs.ArcTolerance = 10e3;
            ofs.AddPaths(board_outline, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            ClipperLib::Paths paths_ofs;
            ofs.Execute(paths_ofs, clearance * -1.0);

            // subtract board outline from patch
            ClipperLib::Clipper clipper;
            clipper.AddPaths(paths_ofs, ClipperLib::ptClip, true);
            clipper.AddPaths(it.second, ClipperLib::ptSubject, true);
            ClipperLib::Paths errors;
            clipper.Execute(ClipperLib::ctDifference, errors, ClipperLib::pftNonZero, ClipperLib::pftEvenOdd);

            // no remaining: no clearance violation
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
                    e.comment = patch_type_names.at(it.first.type) + "(" + (net ? net->name : "") + ") on layer "
                                + brd.get_layer_name(it.first.layer) + " near Board edge";
                    e.error_polygons = {ite};
                    e.add_layer_range(brd, it.first.layer);
                }
            }
        }
    }

    r.update();
    return r;
}
} // namespace horizon
