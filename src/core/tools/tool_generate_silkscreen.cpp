#include "board/board_layers.hpp"
#include "canvas/canvas_gl.hpp"
#include "dialogs/generate_silkscreen_window.hpp"
#include "document/idocument_package.hpp"
#include "imp/imp_interface.hpp"
#include "util/util.hpp"
#include "tool_generate_silkscreen.hpp"
#include "nlohmann/json.hpp"
#include <gdk/gdkkeysyms.h>
#include <sstream>
#include <iomanip>

namespace horizon {

ToolGenerateSilkscreen::ToolGenerateSilkscreen(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

void ToolGenerateSilkscreen::Settings::load_from_json(const json &j)
{
    expand_silk = j.value("expand_silk", .2_mm);
    expand_pad = j.value("expand_pad", .2_mm);
    line_width = j.value("line_width", .15_mm);
}

json ToolGenerateSilkscreen::Settings::serialize() const
{
    json j;
    j["expand_silk"] = expand_silk;
    j["expand_pad"] = expand_pad;
    j["line_width"] = line_width;
    return j;
}

void ToolGenerateSilkscreen::Settings::load_defaults()
{
    load_from_json(json::object({}));
}

bool ToolGenerateSilkscreen::can_begin()
{
    return doc.k;
}

bool ToolGenerateSilkscreen::select_polygon()
{
    bool ret = true;
    auto pkg = doc.k->get_package();
    for (const auto &it : selection) {
        if (it.layer == BoardLayers::TOP_PACKAGE
            && (it.type == ObjectType::POLYGON_EDGE || it.type == ObjectType::POLYGON_VERTEX)) {
            pp = pkg->get_polygon(it.uuid);
        }
    }
    if (!pp) {
        for (const auto &it : pkg->polygons) {
            if (it.second.layer == BoardLayers::TOP_PACKAGE) {
                if (!pp) {
                    pp = &it.second;
                }
                else {
                    ret = false;
                }
            }
        }
    }

    if (!pp) {
        return ret;
    }

    auto package = pp->remove_arcs();

    path_pkg.clear();


    for (const auto &it : package.vertices) {
        path_pkg.emplace_back(it.position.x, it.position.y);
    }
    return ret;
}

void ToolGenerateSilkscreen::clear_silkscreen()
{
    auto pkg = doc.k->get_package();
    map_erase_if(pkg->lines, [](const auto &a) { return a.second.layer == BoardLayers::TOP_SILKSCREEN; });
}

ToolResponse ToolGenerateSilkscreen::redraw_silkscreen()
{
    clear_silkscreen();

    ClipperLib::ClipperOffset ofs_pads;
    ClipperLib::Paths pads_expanded;
    ofs_pads.AddPaths(pads, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    /* additional .000001_mm offset helps with rounding errors */
    ofs_pads.Execute(pads_expanded, settings.expand_pad + (settings.line_width / 2) + .000001_mm);

    ClipperLib::ClipperOffset ofs_pkg;
    ClipperLib::Paths pkg_expanded;
    ofs_pkg.AddPath(path_pkg, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    ofs_pkg.Execute(pkg_expanded, settings.expand_silk + .075_mm);
    if (pkg_expanded.size() != 1) {
        imp->tool_bar_flash("expand failed, aborting");
        return ToolResponse::revert();
    }
    /* turn closed polygon into closed polyline */
    pkg_expanded[0].emplace_back(pkg_expanded[0].at(0));

    ClipperLib::Clipper clip;
    ClipperLib::PolyTree silk_tree;
    ClipperLib::Paths silk_paths;
    clip.AddPaths(pkg_expanded, ClipperLib::ptSubject, false);
    clip.AddPaths(pads_expanded, ClipperLib::ptClip, true);
    clip.Execute(ClipperLib::ctDifference, silk_tree, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    ClipperLib::OpenPathsFromPolyTree(silk_tree, silk_paths);
    for (const auto &silk_path : silk_paths) {
        auto first = doc.r->insert_junction(UUID::random());
        auto from = first;
        first->position = Coordi(silk_path.at(0).X, silk_path.at(0).Y);
        for (const auto &c : silk_path) {
            auto to = from;
            /* loop detection and handling */
            if (silk_path.at(0).X == c.X && silk_path.at(0).Y == c.Y) {
                to = first;
            }
            else {
                to = doc.r->insert_junction(UUID::random());
                to->position = Coordi(c.X, c.Y);
            }
            if (to != from) {
                auto line = doc.r->insert_line(UUID::random());
                line->width = settings.line_width;
                line->layer = BoardLayers::TOP_SILKSCREEN;
                line->from = from;
                line->to = to;
                from = to;
            }
        }
    }

    return ToolResponse();
}

void ToolGenerateSilkscreen::restore_package_visibility()
{
    auto ld = imp->get_layer_display(BoardLayers::TOP_PACKAGE);
    ld.visible = package_visible;
    imp->set_layer_display(BoardLayers::TOP_PACKAGE, ld);
}

ToolResponse ToolGenerateSilkscreen::begin(const ToolArgs &args)
{
    auto pkg = doc.k->get_package();
    pp = nullptr;

    if (!select_polygon()) {
        if (!pp) {
            imp->tool_bar_flash("found no package polygon, aborting");
            return ToolResponse::end();
        }
        else {
            imp->tool_bar_flash("found multiple package polygons, select a different one if necessary");
        }
    }

    for (const auto &it : pkg->pads) {
        for (const auto &it_poly : it.second.padstack.polygons) {
            if (it_poly.second.layer == BoardLayers::TOP_COPPER) {
                ClipperLib::Path pad_path;
                auto polygon = it_poly.second.remove_arcs();
                for (const auto &vertex : polygon.vertices) {
                    auto pos = it.second.placement.transform(vertex.position);
                    pad_path.emplace_back(pos.x, pos.y);
                }
                pads.emplace_back(std::move(pad_path));
            }
        }
        for (const auto &it_shape : it.second.padstack.shapes) {
            if (it_shape.second.layer == BoardLayers::TOP_COPPER) {
                ClipperLib::Path pad_path;
                auto polygon = it_shape.second.to_polygon().remove_arcs();
                for (const auto &vertex : polygon.vertices) {
                    auto pos = it.second.placement.transform(vertex.position);
                    pad_path.emplace_back(pos.x, pos.y);
                }
                pads.emplace_back(std::move(pad_path));
            }
        }
        for (const auto &it_hole : it.second.padstack.holes) {
            ClipperLib::Path pad_path;
            auto polygon = it_hole.second.to_polygon().remove_arcs();
            for (const auto &vertex : polygon.vertices) {
                auto pos = it.second.placement.transform(vertex.position);
                pad_path.emplace_back(pos.x, pos.y);
            }
            pads.emplace_back(std::move(pad_path));
        }
    }
    ClipperLib::Clipper join;
    join.AddPaths(pads, ClipperLib::ptSubject, true);
    join.Execute(ClipperLib::ctUnion, pads, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    update(args);

    {
        auto ld = imp->get_layer_display(BoardLayers::TOP_PACKAGE);
        package_visible = ld.visible;
        ld.visible = true;
        imp->set_layer_display(BoardLayers::TOP_PACKAGE, ld);
    }

    imp->set_work_layer(BoardLayers::TOP_SILKSCREEN);

    win = imp->dialogs.show_generate_silkscreen_window(&settings);

    return ToolResponse();
}

ToolResponse ToolGenerateSilkscreen::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::DATA) {
        if (auto data = dynamic_cast<const ToolDataWindow *>(args.data.get())) {
            if (data->event == ToolDataWindow::Event::CLOSE) {
                restore_package_visibility();
                return ToolResponse::revert();
            }
            else if (data->event == ToolDataWindow::Event::OK) {
                restore_package_visibility();
                return ToolResponse::commit();
            }
            else if (data->event == ToolDataWindow::Event::UPDATE) {
                return redraw_silkscreen();
            }
        }
    }
    else if (args.type == ToolEventType::CLICK && args.button == 1 && args.target.layer == BoardLayers::TOP_PACKAGE
             && (args.target.type == ObjectType::POLYGON_EDGE || args.target.type == ObjectType::POLYGON_VERTEX)) {
        selection.clear();
        selection.emplace(args.target.path.at(0), args.target.type, args.target.vertex, args.target.layer);
        select_polygon();
        return redraw_silkscreen();
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_Return || args.key == GDK_KEY_KP_Enter) {
            restore_package_visibility();
            return ToolResponse::commit();
        }
        else if (args.key == GDK_KEY_Escape) {
            restore_package_visibility();
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::NONE) {
        return redraw_silkscreen();
    }
    return ToolResponse();
}
} // namespace horizon
