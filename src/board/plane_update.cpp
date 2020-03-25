#include "board.hpp"
#include "board_layers.hpp"
#include "canvas/canvas_pads.hpp"
#include "canvas/canvas_patch.hpp"
#include "util/util.hpp"
#include <mutex>
#include <thread>

namespace horizon {

static void polynode_to_fragment(Plane *plane, const ClipperLib::PolyNode *node)
{
    assert(node->IsHole() == false);
    plane->fragments.emplace_back();
    auto &fragment = plane->fragments.back();
    fragment.paths.emplace_back();
    auto &outer = fragment.paths.back();
    outer = node->Contour; // first path is countour

    for (auto child : node->Childs) {
        assert(child->IsHole() == true);

        fragment.paths.emplace_back();
        auto &hole = fragment.paths.back();
        hole = child->Contour;

        for (auto child2 : child->Childs) { // add fragments in holes
            polynode_to_fragment(plane, child2);
        }
    }
}

static void transform_path(ClipperLib::Path &path, const Placement &tr)
{
    for (auto &it : path) {
        Coordi p(it.X, it.Y);
        auto q = tr.transform(p);
        it.X = q.x;
        it.Y = q.y;
    }
}

static std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> get_path_bb(const ClipperLib::Path &path)
{
    std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> bb;
    bb.first = path.front();
    bb.second = bb.first;
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
    std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> bb;
    bb.first = paths.front().front();
    bb.second = bb.first;
    for (const auto &path : paths) {
        for (const auto &pt : path) {
            bb.first.X = std::min(bb.first.X, pt.X);
            bb.first.Y = std::min(bb.first.Y, pt.Y);
            bb.second.X = std::max(bb.second.X, pt.X);
            bb.second.Y = std::max(bb.second.Y, pt.Y);
        }
    }
    return bb;
}

static bool bb_isect(const std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> &bb1,
                     const std::pair<ClipperLib::IntPoint, ClipperLib::IntPoint> &bb2)
{
    return (bb1.second.X >= bb2.first.X && bb2.second.X >= bb1.first.X)
           && (bb1.second.Y >= bb2.first.Y && bb2.second.Y >= bb1.first.Y);
}

void Board::update_plane(Plane *plane, const CanvasPatch *ca_ext, const CanvasPads *ca_pads_ext)
{
    CanvasPatch ca_my;
    const CanvasPatch *ca = ca_ext;
    if (!ca_ext) {
        ca_my.update(*this, Canvas::PanelMode::SKIP);
        ca = &ca_my;
    }

    CanvasPads ca_pads_my;
    const CanvasPads *ca_pads = ca_pads_ext;
    if (!ca_pads_ext) {
        ca_pads_my.update(*this, Canvas::PanelMode::SKIP);
        ca_pads = &ca_pads_my;
    }

    plane->fragments.clear();
    plane->revision++;

    ClipperLib::Clipper cl_plane;
    ClipperLib::Path poly_path; // path from polygon contour
    auto poly = plane->polygon->remove_arcs();
    poly_path.reserve(poly.vertices.size());
    for (const auto &pt : poly.vertices) {
        poly_path.emplace_back(ClipperLib::IntPoint(pt.position.x, pt.position.y));
    }

    auto poly_bb = get_path_bb(poly_path);

    ClipperLib::JoinType jt = ClipperLib::jtRound;
    switch (plane->settings.style) {
    case PlaneSettings::Style::ROUND:
        jt = ClipperLib::jtRound;
        break;

    case PlaneSettings::Style::SQUARE:
        jt = ClipperLib::jtSquare;
        break;

    case PlaneSettings::Style::MITER:
        jt = ClipperLib::jtMiter;
        break;
    }

    {
        ClipperLib::ClipperOffset ofs_poly; // shrink polygon contour by min_width/2
        ofs_poly.ArcTolerance = 2e3;
        ofs_poly.AddPath(poly_path, jt, ClipperLib::etClosedPolygon);
        ClipperLib::Paths poly_shrink;
        ofs_poly.Execute(poly_shrink, -((double)plane->settings.min_width) / 2);
        cl_plane.AddPaths(poly_shrink, ClipperLib::ptSubject, true);
    }

    double twiddle = .005_mm;

    for (const auto &patch : ca->get_patches()) { // add cutouts
        if ((patch.first.layer == poly.layer && patch.first.net != plane->net->uuid && patch.second.size()
             && patch.first.type != PatchType::OTHER && patch.first.type != PatchType::TEXT)
            || (patch.first.layer == 10000 && patch.first.type == PatchType::HOLE_NPTH)) {

            int64_t clearance = 0;
            if (patch.first.type != PatchType::HOLE_NPTH) { // copper
                auto patch_net = patch.first.net ? &block->nets.at(patch.first.net) : nullptr;
                auto rule_clearance = rules.get_clearance_copper(plane->net, patch_net, poly.layer);
                if (rule_clearance) {
                    clearance = rule_clearance->get_clearance(patch.first.type, PatchType::PLANE);
                }
            }
            else { // npth
                clearance = rules.get_clearance_copper_other(plane->net, poly.layer)
                                    ->get_clearance(PatchType::PLANE, PatchType::HOLE_NPTH);
            }

            auto patch_bb = get_paths_bb(patch.second);
            patch_bb.first.X -= 2 * clearance;
            patch_bb.first.Y -= 2 * clearance;
            patch_bb.second.X += 2 * clearance;
            patch_bb.second.Y += 2 * clearance;

            if (bb_isect(poly_bb, patch_bb)) {
                ClipperLib::ClipperOffset ofs; // expand patch for cutout
                ofs.ArcTolerance = 2e3;
                ofs.AddPaths(patch.second, jt, ClipperLib::etClosedPolygon);
                ClipperLib::Paths patch_exp;

                double expand = clearance + plane->settings.min_width / 2 + twiddle;

                ofs.Execute(patch_exp, expand);
                cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
            }
        }
    }

    auto text_clearance =
            rules.get_clearance_copper_other(plane->net, poly.layer)->get_clearance(PatchType::PLANE, PatchType::TEXT);

    // add text cutouts
    if (plane->settings.text_style == PlaneSettings::TextStyle::EXPAND) {
        for (const auto &patch : ca->get_patches()) { // add cutouts
            if (patch.first.layer == poly.layer && patch.first.type == PatchType::TEXT) {
                ClipperLib::ClipperOffset ofs; // expand patch for cutout
                ofs.ArcTolerance = 2e3;
                ofs.AddPaths(patch.second, jt, ClipperLib::etClosedPolygon);
                ClipperLib::Paths patch_exp;

                double expand = text_clearance + plane->settings.min_width / 2 + twiddle;

                ofs.Execute(patch_exp, expand);
                cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
            }
        }
    }
    else {
        for (const auto &ext : ca->get_text_extents()) { // add cutouts
            Coordi a, b;
            int text_layer;
            std::tie(text_layer, a, b) = ext;
            if (text_layer == poly.layer) {
                ClipperLib::Path p = {{a.x, a.y}, {b.x, a.y}, {b.x, b.y}, {a.x, b.y}};

                ClipperLib::ClipperOffset ofs; // expand patch for cutout
                ofs.ArcTolerance = 2e3;
                ofs.AddPath(p, jt, ClipperLib::etClosedPolygon);
                ClipperLib::Paths patch_exp;

                double expand = text_clearance + plane->settings.min_width / 2 + twiddle;

                ofs.Execute(patch_exp, expand);
                cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
            }
        }
    }

    // add keepouts
    {
        auto keepout_rules = dynamic_cast_vector<RuleClearanceCopperKeepout *>(
                rules.get_rules_sorted(RuleID::CLEARANCE_COPPER_KEEPOUT));
        auto keepout_contours = get_keepout_contours();
        for (const auto &it_keepout : keepout_contours) {
            const auto keepout = it_keepout.keepout;
            if ((plane->polygon->layer == keepout->polygon->layer || keepout->all_cu_layers)
                && keepout->patch_types_cu.count(PatchType::PLANE)) {
                auto clearance =
                        rules.get_clearance_copper_keepout(plane->net, &it_keepout)->get_clearance(PatchType::PLANE);

                ClipperLib::Paths keepout_contour_expanded;
                ClipperLib::ClipperOffset ofs;
                ofs.ArcTolerance = 10e3;
                ofs.AddPath(it_keepout.contour, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
                ofs.Execute(keepout_contour_expanded, clearance + plane->settings.min_width / 2 + .05_mm);
                cl_plane.AddPaths(keepout_contour_expanded, ClipperLib::ptClip, true);
            }
        }
    }

    // add thermal coutouts
    if (plane->settings.connect_style == PlaneSettings::ConnectStyle::THERMAL) {
        for (const auto &patch : ca->get_patches()) {
            if ((patch.first.layer == poly.layer && patch.first.net == plane->net->uuid
                 && (patch.first.type == PatchType::PAD || patch.first.type == PatchType::PAD_TH))) {
                ClipperLib::ClipperOffset ofs; // expand patch for cutout
                ofs.ArcTolerance = 2e3;
                ClipperLib::Paths pad;
                ClipperLib::SimplifyPolygons(patch.second, pad, ClipperLib::pftNonZero);

                ofs.AddPaths(pad, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
                ClipperLib::Paths patch_exp;

                double expand = plane->settings.thermal_gap_width + plane->settings.min_width / 2;
                ofs.Execute(patch_exp, expand);
                cl_plane.AddPaths(patch_exp, ClipperLib::ptClip, true);
            }
        }
    }
    ClipperLib::Paths out;                                                   // the plane before expanding by min_width
    cl_plane.Execute(ClipperLib::ctDifference, out, ClipperLib::pftNonZero); // do cutouts

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
            ofs.AddPaths(board_outline, jt, ClipperLib::etClosedPolygon);
            auto clearance = rules.get_clearance_copper_other(plane->net, poly.layer)
                                     ->get_clearance(PatchType::PLANE, PatchType::BOARD_EDGE);
            ofs.Execute(paths_ofs, -1.0 * (clearance + plane->settings.min_width / 2 + twiddle * 2));
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

    ClipperLib::Paths final;
    ClipperLib::PolyTree tree;
    ClipperLib::ClipperOffset ofs;
    ofs.ArcTolerance = 2e3;
    ofs.AddPaths(out, jt, ClipperLib::etClosedPolygon);

    if (plane->settings.connect_style == PlaneSettings::ConnectStyle::THERMAL) {
        ClipperLib::Paths expanded;
        ofs.Execute(expanded, plane->settings.min_width / 2);

        ClipperLib::Clipper cl_add_spokes;
        cl_add_spokes.AddPaths(expanded, ClipperLib::ptSubject, true);
        auto thermals = get_thermals(plane, ca_pads);
        for (const auto &spoke : thermals) {
            ClipperLib::Paths test_result;
            ClipperLib::Clipper cl_test;
            cl_test.AddPaths(expanded, ClipperLib::ptSubject, true);
            cl_test.AddPath(spoke, ClipperLib::ptClip, true);
            cl_test.Execute(ClipperLib::ctIntersection, test_result, ClipperLib::pftNonZero);

            if (test_result.size())
                cl_add_spokes.AddPath(spoke, ClipperLib::ptClip, true);
        }
        if (plane->settings.fill_style == PlaneSettings::FillStyle::SOLID) {
            cl_add_spokes.Execute(ClipperLib::ctUnion, tree, ClipperLib::pftNonZero);
        }
        else {
            cl_add_spokes.Execute(ClipperLib::ctUnion, final, ClipperLib::pftNonZero);
        }
    }
    else {
        if (plane->settings.fill_style == PlaneSettings::FillStyle::SOLID) {
            ofs.Execute(tree, plane->settings.min_width / 2);
        }
        else {
            ofs.Execute(final, plane->settings.min_width / 2);
        }
    }

    if (plane->settings.fill_style == PlaneSettings::FillStyle::HATCH) {
        ClipperLib::Paths contracted;
        {
            ClipperLib::ClipperOffset cl;
            cl.ArcTolerance = 2e3;
            cl.AddPaths(final, ClipperLib::jtRound, ClipperLib::etClosedPolygon);
            cl.Execute(contracted, -1.0 * plane->settings.hatch_border_width);
        }

        ClipperLib::Paths border;
        {
            ClipperLib::Clipper cl;
            cl.AddPaths(final, ClipperLib::ptSubject, true);
            cl.AddPaths(contracted, ClipperLib::ptClip, true);
            cl.Execute(ClipperLib::ctDifference, border, ClipperLib::pftNonZero);
        }

        std::pair<Coordi, Coordi> bb;
        bb.first = poly.vertices.front().position;
        bb.second = bb.first;
        for (const auto &it : poly.vertices) {
            bb.first = Coordi::min(bb.first, it.position);
            bb.second = Coordi::max(bb.second, it.position);
        }
        ClipperLib::Paths grid;
        const int64_t line_width = plane->settings.hatch_line_width;
        const int64_t line_spacing = plane->settings.hatch_line_spacing;

        for (int64_t x = bb.first.x; x < bb.second.x; x += line_spacing) {
            ClipperLib::Path line(4);
            line[0] = {x - line_width / 2, bb.first.y};
            line[1] = {x + line_width / 2, bb.first.y};
            line[2] = {x + line_width / 2, bb.second.y};
            line[3] = {x - line_width / 2, bb.second.y};
            grid.push_back(line);
        }
        for (int64_t y = bb.first.y; y < bb.second.y; y += line_spacing) {
            ClipperLib::Path line(4);
            line[0] = {bb.first.x, y + line_width / 2};
            line[1] = {bb.first.x, y - line_width / 2};
            line[2] = {bb.second.x, y - line_width / 2};
            line[3] = {bb.second.x, y + line_width / 2};
            grid.push_back(line);
        }

        ClipperLib::Paths grid_isect;
        {
            ClipperLib::Clipper cl;
            cl.AddPaths(grid, ClipperLib::ptSubject, true);
            cl.AddPaths(final, ClipperLib::ptClip, true);
            cl.Execute(ClipperLib::ctIntersection, grid_isect, ClipperLib::pftNonZero);
        }
        {
            ClipperLib::Clipper cl;
            cl.AddPaths(grid_isect, ClipperLib::ptSubject, true);
            cl.AddPaths(border, ClipperLib::ptClip, true);
            cl.Execute(ClipperLib::ctUnion, tree, ClipperLib::pftNonZero);
        }
    }

    for (const auto node : tree.Childs) {
        polynode_to_fragment(plane, node);
    }

    // orphan all
    for (auto &it : plane->fragments) {
        it.orphan = true;
    }

    // find orphans
    for (auto &frag : plane->fragments) {
        for (const auto &it : junctions) {
            if (it.second.net == plane->net && (it.second.layer == plane->polygon->layer || it.second.has_via)
                && frag.contains(it.second.position)) {
                frag.orphan = false;
                break;
            }
        }
        if (frag.orphan) { // still orphan, check pads
            for (auto &it : packages) {
                auto pkg = &it.second;
                for (auto &it_pad : it.second.package.pads) {
                    auto pad = &it_pad.second;
                    if (pad->net == plane->net) {
                        Track::Connection conn(pkg, pad);
                        auto pos = conn.get_position();
                        auto ps_type = pad->padstack.type;
                        auto layer = plane->polygon->layer;
                        if (frag.contains(pos)
                            && (ps_type == Padstack::Type::THROUGH
                                || (ps_type == Padstack::Type::TOP && layer == BoardLayers::TOP_COPPER)
                                || (ps_type == Padstack::Type::BOTTOM && layer == BoardLayers::BOTTOM_COPPER))) {
                            frag.orphan = false;
                            break;
                        }
                    }
                }
                if (!frag.orphan) { // don't need to check other packages
                    break;
                }
            }
        }
    }

    if (!plane->settings.keep_orphans) { // delete orphans
        plane->fragments.erase(std::remove_if(plane->fragments.begin(), plane->fragments.end(),
                                              [](const auto &x) { return x.orphan; }),
                               plane->fragments.end());
    }
}

static void plane_update_worker(std::mutex &mutex, std::set<Plane *> &planes, Board *brd, const CanvasPatch *ca_patch,
                                const CanvasPads *ca_pads)
{
    while (true) {
        Plane *plane = nullptr;
        {
            std::lock_guard<std::mutex> guard(mutex);
            if (planes.size() == 0)
                break;
            auto it = planes.begin();
            plane = *it;
            planes.erase(it);
        }
        assert(plane);
        brd->update_plane(plane, ca_patch, ca_pads);
    }
}

void Board::update_planes()
{
    std::map<int, std::set<Plane *>> planes_by_priority;
    for (auto &it : planes) {
        it.second.fragments.clear();
        planes_by_priority[it.second.priority].insert(&it.second);
    }
    std::vector<int> plane_priorities;
    std::transform(planes_by_priority.begin(), planes_by_priority.end(), std::back_inserter(plane_priorities),
                   [](const auto &a) { return a.first; });
    std::sort(plane_priorities.begin(), plane_priorities.end());

    CanvasPads cp;
    cp.update(*this, Canvas::PanelMode::SKIP);

    CanvasPatch ca;
    ca.update(*this, Canvas::PanelMode::SKIP);

    for (auto priority : plane_priorities) {
        std::mutex plane_update_mutex;
        std::set<Plane *> planes_for_workers = planes_by_priority.at(priority);

        if (planes_for_workers.size() >= std::thread::hardware_concurrency()) {
            std::vector<std::thread> threads;
            for (size_t i = 0; i < std::thread::hardware_concurrency(); i++) {
                threads.emplace_back(plane_update_worker, std::ref(plane_update_mutex), std::ref(planes_for_workers),
                                     this, &ca, &cp);
            }
            for (auto &thr : threads) {
                thr.join();
            }
        }
        else {
            for (auto plane : planes_by_priority.at(priority)) {
                update_plane(plane, &ca, &cp);
            }
        }
        for (auto plane : planes_by_priority.at(priority)) {
            ca.append_polygon(*(plane->polygon));
        }
    }
}

ClipperLib::Paths Board::get_thermals(Plane *plane, const CanvasPads *cp) const
{
    ClipperLib::Paths ret;
    for (const auto &it : cp->pads) {
        if (it.first.layer != plane->polygon->layer)
            continue;

        auto net_from_pad = packages.at(it.first.package).package.pads.at(it.first.pad).net;
        if (net_from_pad != plane->net)
            continue;

        ClipperLib::Paths uni; // union of all off the pads polygons
        ClipperLib::SimplifyPolygons(it.second.second, uni, ClipperLib::pftNonZero);

        ClipperLib::ClipperOffset ofs; //
        ofs.ArcTolerance = 2e3;
        ofs.AddPaths(uni, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
        ClipperLib::Paths pad_exp; // pad expanded by gap width

        double expand = plane->settings.thermal_gap_width + .01_mm;
        ofs.Execute(pad_exp, expand);

        auto p0 = pad_exp.front().front();
        Coordi pc0(p0.X, p0.Y);
        std::pair<Coordi, Coordi> bb(pc0, pc0);
        for (const auto &itp : pad_exp) {
            for (const auto &itc : itp) {
                Coordi p(itc.X, itc.Y);
                bb.first = Coordi::min(bb.first, p);
                bb.second = Coordi::max(bb.second, p);
            }
        }
        auto w = bb.second.x - bb.first.x;
        auto h = bb.second.y - bb.first.y;
        auto l = std::max(w, h);

        int64_t spoke_width = plane->settings.thermal_spoke_width;
        ClipperLib::Path spoke;
        spoke.emplace_back(-spoke_width / 2, -spoke_width / 2);
        spoke.emplace_back(-spoke_width / 2, spoke_width / 2);
        spoke.emplace_back(l, spoke_width / 2);
        spoke.emplace_back(l, -spoke_width / 2);

        ClipperLib::Paths antipad;
        {

            for (int angle = 0; angle < 360; angle += 90) {
                ClipperLib::Clipper cl;
                cl.AddPaths(pad_exp, ClipperLib::ptSubject, true);

                ClipperLib::Path r1 = spoke;
                Placement tr;
                tr.set_angle_deg(angle);
                transform_path(r1, tr);
                transform_path(r1, it.second.first);
                cl.AddPath(r1, ClipperLib::ptClip, true);

                cl.Execute(ClipperLib::ctIntersection, antipad, ClipperLib::pftNonZero);
                ret.insert(ret.end(), antipad.begin(), antipad.end());
            }
        }
    }
    return ret;
}
} // namespace horizon
