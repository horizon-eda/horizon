#include "board.hpp"
#include "board_layers.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"

namespace horizon {

class MyCanvasPatch : public CanvasPatch {
public:
    using CanvasPatch::CanvasPatch;

protected:
    bool img_layer_is_visible(const LayerRange &layer) const override
    {
        return BoardLayers::is_copper(layer.start()) || BoardLayers::is_copper(layer.end())
               || layer == BoardLayers::L_OUTLINE || layer == 10000;
    }
};

static std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> get_path_bb(const ClipperLib::Path &path)
{
    auto bb = std::make_pair(path.front(), path.front());
    for (const auto &pt : path) {
        bb.first.X = std::min(bb.first.X, pt.X);
        bb.first.Y = std::min(bb.first.Y, pt.Y);
        bb.second.X = std::max(bb.second.X, pt.X);
        bb.second.Y = std::max(bb.second.Y, pt.Y);
    }
    return bb;
}

static std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> get_paths_bb(const ClipperLib::Paths &paths)
{
    auto bb = std::make_pair(paths.front().front(), paths.front().front());
    for (const auto &path : paths)
        for (const auto &pt : path) {
            bb.first.X = std::min(bb.first.X, pt.X);
            bb.first.Y = std::min(bb.first.Y, pt.Y);
            bb.second.X = std::max(bb.second.X, pt.X);
            bb.second.Y = std::max(bb.second.Y, pt.Y);
        }
    return bb;
}

static bool bb_isect(const std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> &a,
                     const std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> &b)
{
    return a.second.X >= b.first.X && b.second.X >= a.first.X && a.second.Y >= b.first.Y && b.second.Y >= a.first.Y;
}

bool Board::update_copper_thief(CopperThief *thief, const CanvasPatch *ca_ext, const std::atomic_bool &cancel)
{
    // clear before building the canvas, otherwise the thief's own fragments would become cutouts
    thief->clear();

    MyCanvasPatch ca_my;
    const CanvasPatch *ca = ca_ext;
    if (!ca_ext) {
        ca_my.update(*this, Canvas::PanelMode::SKIP);
        ca = &ca_my;
    }

    auto poly = thief->polygon->remove_arcs();
    const double twiddle = .005_mm;
    ClipperLib::Paths out;
    {
        ClipperLib::Clipper cl_plane;
        ClipperLib::Path poly_path; // path from polygon contour

        poly_path.reserve(poly.vertices.size());
        for (const auto &pt : poly.vertices) {
            poly_path.emplace_back(ClipperLib::IntPoint(pt.position.x, pt.position.y));
        }

        auto poly_bb = get_path_bb(poly_path);
        cl_plane.AddPath(poly_path, ClipperLib::ptSubject, true);

        if (cancel)
            return false;

        // copper thieving isn't connected to anything, so it keeps its distance from all copper
        for (const auto &patch : ca->get_patches()) {
            if ((patch.first.layer == poly.layer && patch.second.size() && patch.first.type != PatchType::OTHER
                 && patch.first.type != PatchType::TEXT)
                || ((patch.first.layer.overlaps(poly.layer))
                    && (patch.first.type == PatchType::HOLE_NPTH || patch.first.type == PatchType::HOLE_PTH))) {
                if (patch.first.type == PatchType::COPPER_THIEF && patch.first.net == thief->uuid)
                    continue;
                if (cancel)
                    return false;
                int64_t clearance = 0;
                if (patch.first.type != PatchType::HOLE_NPTH) { // copper
                    auto patch_net = patch.first.net && patch.first.type != PatchType::COPPER_THIEF
                                             ? &block->nets.at(patch.first.net)
                                             : nullptr;
                    const auto &rule_clearance = rules.get_clearance_copper(nullptr, patch_net, poly.layer);
                    clearance = rule_clearance.get_clearance(patch.first.type, PatchType::COPPER_THIEF);
                }
                else { // npth
                    clearance = rules.get_clearance_copper_other(nullptr, poly.layer)
                                        .get_clearance(PatchType::COPPER_THIEF, PatchType::HOLE_NPTH);
                }

                auto patch_bb = get_paths_bb(patch.second);
                patch_bb.first.X -= 2 * clearance;
                patch_bb.first.Y -= 2 * clearance;
                patch_bb.second.X += 2 * clearance;
                patch_bb.second.Y += 2 * clearance;

                if (bb_isect(poly_bb, patch_bb)) {
                    ClipperLib::ClipperOffset ofs; // expand patch for cutout
                    ofs.ArcTolerance = 2e3;
                    ofs.AddPaths(patch.second, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                    ClipperLib::Paths patch_exp;

                    ofs.Execute(patch_exp, clearance + twiddle);
                    cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
                }
            }
        }

        // add text cutouts
        auto text_clearance = rules.get_clearance_copper_other(nullptr, poly.layer)
                                      .get_clearance(PatchType::COPPER_THIEF, PatchType::TEXT);
        for (const auto &patch : ca->get_patches()) {
            if (patch.first.layer == poly.layer && patch.first.type == PatchType::TEXT) {
                ClipperLib::ClipperOffset ofs; // expand patch for cutout
                ofs.ArcTolerance = 2e3;
                ofs.AddPaths(patch.second, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                ClipperLib::Paths patch_exp;

                ofs.Execute(patch_exp, text_clearance + twiddle);
                cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
            }
        }

        // add keepouts
        for (const auto &it_keepout : get_keepout_contours()) {
            const auto keepout = it_keepout.keepout;
            if ((poly.layer == keepout->polygon->layer || keepout->all_cu_layers)
                && keepout->patch_types_cu.count(PatchType::COPPER_THIEF)) {
                if (cancel)
                    return false;
                auto clearance =
                        rules.get_clearance_copper_keepout(nullptr, &it_keepout).get_clearance(PatchType::COPPER_THIEF);

                ClipperLib::Paths keepout_contour_expanded;
                ClipperLib::ClipperOffset ofs;
                ofs.ArcTolerance = 10e3;
                ofs.AddPath(it_keepout.contour, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                ofs.Execute(keepout_contour_expanded, clearance + .05_mm);
                cl_plane.AddPaths(keepout_contour_expanded, ClipperLib::ptClip, true);
            }
        }

        cl_plane.Execute(ClipperLib::ctDifference, out, ClipperLib::pftNonZero); // do cutouts
    }

    // do board outline clearance
    CanvasPatch::PatchKey outline_key;
    outline_key.layer = BoardLayers::L_OUTLINE;
    outline_key.net = UUID();
    outline_key.type = PatchType::OTHER;
    if (ca->get_patches().count(outline_key) != 0) {
        auto &patch_outline = ca->get_patches().at(outline_key);
        // cleanup board outline so that it conforms to nonzero filling rule
        ClipperLib::Paths board_outline;
        {
            ClipperLib::Clipper cl_outline;
            cl_outline.AddPaths(patch_outline, ClipperLib::ptSubject, true);
            cl_outline.Execute(ClipperLib::ctUnion, board_outline, ClipperLib::pftEvenOdd);
        }

        // board outline contracted by clearance
        ClipperLib::Paths paths_ofs;
        {
            ClipperLib::ClipperOffset ofs;
            ofs.ArcTolerance = 10e3;
            ofs.AddPaths(board_outline, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            auto clearance = rules.get_clearance_copper_other(nullptr, poly.layer)
                                     .get_clearance(PatchType::COPPER_THIEF, PatchType::BOARD_EDGE);
            ofs.Execute(paths_ofs, -1.0 * (clearance + twiddle * 2));
        }

        // intersect polygon with contracted board outline
        ClipperLib::Paths temp;
        {
            ClipperLib::Clipper isect;
            isect.AddPaths(paths_ofs, ClipperLib::ptClip, true);
            isect.AddPaths(out, ClipperLib::ptSubject, true);
            isect.Execute(ClipperLib::ctIntersection, temp, ClipperLib::pftNonZero);
        }
        out = temp;
    }

    // thieving pads are isolated by design, so they're never orphans
    thief->regenerate(out);
    return true;
}

void Board::update_copper_thieves(const std::atomic_bool &cancel)
{
    // clear all first, so that no thief turns another one's copper into cutouts
    for (auto &[uu, thief] : copper_thieves) {
        thief.clear();
    }
    MyCanvasPatch ca;
    ca.update(*this, Canvas::PanelMode::SKIP);

    for (auto &[uu, thief] : copper_thieves) {
        if (cancel)
            return;
        if (update_copper_thief(&thief, &ca, cancel)) {
            // Make generated copper visible to the next thief in this batch so
            // neighboring thieving areas receive the same clearance treatment.
            ca.append_polygon(*thief.polygon);
        }
    }
}
} // namespace horizon
