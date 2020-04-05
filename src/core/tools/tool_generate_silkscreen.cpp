#include "tool_generate_silkscreen.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "board/board_layers.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolGenerateSilkscreen::ToolGenerateSilkscreen(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}


bool ToolGenerateSilkscreen::can_begin()
{
    return doc.k;
}

ToolResponse ToolGenerateSilkscreen::begin(const ToolArgs &args)
{
    Coordi a, b;
    auto pkg = doc.k->get_package();
    const Polygon *pp = NULL;
    int64_t expand = .225_mm;

    imp->set_work_layer(BoardLayers::TOP_SILKSCREEN);

    for (const auto &it : pkg->polygons) {
        if (it.second.layer == BoardLayers::TOP_PACKAGE) {
            if (!pp) {
                pp = &it.second;
            }
            else {
                imp->tool_bar_flash("found multiple package polygons, aborting");
                return ToolResponse::end();
            }
        }
    }

    if (!pp) {
        imp->tool_bar_flash("found no package polygon, aborting");
        return ToolResponse::end();
    }
    auto package = pp->remove_arcs();

    ClipperLib::Path path;
    for (const auto &it : package.vertices) {
        path.emplace_back(ClipperLib::IntPoint(it.position.x, it.position.y));
    }

    ClipperLib::ClipperOffset ofs;
    ClipperLib::Paths expanded_paths;
    ofs.AddPath(path, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    ofs.Execute(expanded_paths, expand);
    if (expanded_paths.size() != 1) {
        imp->tool_bar_flash("expand failed, aborting");
        return ToolResponse::end();
    }
    /* turn closed polygon into closed polyline */
    expanded_paths[0].emplace_back(expanded_paths[0].at(0));

    ClipperLib::Paths pads;
    for (const auto &it : pkg->pads) {
        for (const auto &it_poly : it.second.padstack.polygons) {
            if (it_poly.second.layer == BoardLayers::TOP_COPPER) {
                ClipperLib::Path pad_path;
                ClipperLib::Paths pad_paths;
                ClipperLib::ClipperOffset pad_ofs;
                auto polygon = it_poly.second.remove_arcs();
                for (const auto &vertex : polygon.vertices) {
                    auto pos = it.second.placement.transform(vertex.position);
                    pad_path.emplace_back(ClipperLib::IntPoint(pos.x, pos.y));
                }
                pad_ofs.AddPath(pad_path, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
                pad_ofs.Execute(pad_paths, expand);
                for (const auto &it_path : pad_paths) {
                    pads.emplace_back(it_path);
                }
            }
        }
        for (const auto &it_shape : it.second.padstack.shapes) {
            if (it_shape.second.layer == BoardLayers::TOP_COPPER) {
                ClipperLib::Path pad_path;
                ClipperLib::Paths pad_paths;
                ClipperLib::ClipperOffset pad_ofs;
                auto polygon = it_shape.second.to_polygon().remove_arcs();
                for (const auto &vertex : polygon.vertices) {
                    auto pos = it.second.placement.transform(vertex.position);
                    pad_path.emplace_back(ClipperLib::IntPoint(pos.x, pos.y));
                }
                pad_ofs.AddPath(pad_path, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
                pad_ofs.Execute(pad_paths, expand);
                for (const auto &it_path : pad_paths) {
                    pads.emplace_back(it_path);
                }
            }
        }
    }
    ClipperLib::Clipper join;
    join.AddPaths(pads, ClipperLib::ptSubject, true);
    join.Execute(ClipperLib::ctUnion, pads, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    ClipperLib::Clipper clip;
    ClipperLib::PolyTree silk_tree;
    ClipperLib::Paths silk_paths;
    clip.AddPaths(expanded_paths, ClipperLib::ptSubject, false);
    clip.AddPaths(pads, ClipperLib::ptClip, true);
    clip.Execute(ClipperLib::ctDifference, silk_tree, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    ClipperLib::OpenPathsFromPolyTree(silk_tree, silk_paths);
    for (const auto &silk_path : silk_paths) {
        auto uu_j0 = UUID::random();
        auto first = &pkg->junctions.emplace(uu_j0, uu_j0).first->second;
        auto from = first;
        from->position = Coordi(silk_path.at(0).X, silk_path.at(0).Y);
        for (const auto &c : silk_path) {
            auto uu_j = UUID::random();
            auto uu_l = UUID::random();
            auto to = from;
            /* loop detection and handling */
            if (silk_path.at(0).X == c.X && silk_path.at(0).Y == c.Y) {
                to = first;
            }
            else {
                to = &pkg->junctions.emplace(uu_j, uu_j).first->second;
                to->position = Coordi(c.X, c.Y);
            }
            auto line = &pkg->lines.emplace(uu_l, uu_l).first->second;
            line->width = .15_mm;
            line->layer = BoardLayers::TOP_SILKSCREEN;
            line->from = from;
            line->to = to;
            from = to;
        }
    }

    return ToolResponse::commit();
}

ToolResponse ToolGenerateSilkscreen::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
