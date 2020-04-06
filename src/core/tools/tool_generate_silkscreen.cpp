#include "tool_generate_silkscreen.hpp"
#include "document/idocument_package.hpp"
#include "board/board_layers.hpp"
#include "imp/imp_interface.hpp"
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
}

json ToolGenerateSilkscreen::Settings::serialize() const
{
    json j;
    j["expand_silk"] = expand_silk;
    j["expand_pad"] = expand_pad;
    return j;
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

    auto package = pp->remove_arcs();

    path_pkg.clear();


    for (const auto &it : package.vertices) {
        path_pkg.emplace_back(ClipperLib::IntPoint(it.position.x, it.position.y));
    }
    return ret;
}

void ToolGenerateSilkscreen::update_tip()
{
    std::stringstream ss;
    ss << std::fixed << std::setprecision(3);
    ss << "<b>o:</b>adjust outline expansion <b>p:</b>adjust pad expansion <b>Return:</b>finish" << std::endl;
    switch (adjust) {
    case Adjust::SILK:
        ss << "<b>+/-:</b>outline expansion (" << (1.0 * settings.expand_silk / 1_mm) << " mm)";
        break;
    case Adjust::PAD:
        ss << "<b>+/-:</b>pad expansion (" << (1.0 * settings.expand_pad / 1_mm) << " mm)";
        break;
    }
    imp->tool_bar_set_tip(ss.str());
}

void ToolGenerateSilkscreen::clear_silkscreen()
{
    auto pkg = doc.k->get_package();
    std::list<UUID> silklines;
    for (const auto &it : pkg->lines) {
        if (it.second.layer == BoardLayers::TOP_SILKSCREEN) {
            silklines.push_back(it.first);
        }
    }
    for (const auto &it : silklines) {
        doc.r->delete_line(it);
    }
}

ToolResponse ToolGenerateSilkscreen::begin(const ToolArgs &args)
{
    auto pkg = doc.k->get_package();
    pp = nullptr;
    first_update = true;

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
                    pad_path.emplace_back(ClipperLib::IntPoint(pos.x, pos.y));
                }
                pads.emplace_back(pad_path);
            }
        }
        for (const auto &it_shape : it.second.padstack.shapes) {
            if (it_shape.second.layer == BoardLayers::TOP_COPPER) {
                ClipperLib::Path pad_path;
                auto polygon = it_shape.second.to_polygon().remove_arcs();
                for (const auto &vertex : polygon.vertices) {
                    auto pos = it.second.placement.transform(vertex.position);
                    pad_path.emplace_back(ClipperLib::IntPoint(pos.x, pos.y));
                }
                pads.emplace_back(pad_path);
            }
        }
        for (const auto &it_hole : it.second.padstack.holes) {
            ClipperLib::Path pad_path;
            auto polygon = it_hole.second.to_polygon().remove_arcs();
            for (const auto &vertex : polygon.vertices) {
                auto pos = it.second.placement.transform(vertex.position);
                pad_path.emplace_back(ClipperLib::IntPoint(pos.x, pos.y));
            }
            pads.emplace_back(pad_path);
        }
    }
    ClipperLib::Clipper join;
    join.AddPaths(pads, ClipperLib::ptSubject, true);
    join.Execute(ClipperLib::ctUnion, pads, ClipperLib::pftNonZero, ClipperLib::pftNonZero);

    update(args);

    return ToolResponse::change_layer(BoardLayers::TOP_SILKSCREEN);
}

ToolResponse ToolGenerateSilkscreen::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::CLICK && args.button == 1 && args.target.layer == BoardLayers::TOP_PACKAGE
        && (args.target.type == ObjectType::POLYGON_EDGE || args.target.type == ObjectType::POLYGON_VERTEX)) {
        selection.clear();
        selection.emplace(
                SelectableRef(args.target.path.at(0), args.target.type, args.target.vertex, args.target.layer));
        select_polygon();
    }
    else if (first_update) {
        first_update = false;
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_plus || args.key == GDK_KEY_KP_Add) {
            switch (adjust) {
            case Adjust::SILK:
                settings.expand_silk += .05_mm;
                break;
            case Adjust::PAD:
                settings.expand_pad += .05_mm;
                break;
            }
        }
        else if (args.key == GDK_KEY_minus || args.key == GDK_KEY_KP_Subtract) {
            switch (adjust) {
            case Adjust::SILK:
                settings.expand_silk -= .05_mm;
                break;
            case Adjust::PAD:
                settings.expand_pad -= .05_mm;
                break;
            }
        }
        else if (args.key == GDK_KEY_o) {
            adjust = Adjust::SILK;
        }
        else if (args.key == GDK_KEY_p) {
            adjust = Adjust::PAD;
        }
        else if (args.key == GDK_KEY_Return || args.key == GDK_KEY_KP_Enter) {
            return ToolResponse::commit();
        }
        else if (args.key == GDK_KEY_Escape) {
            return ToolResponse::revert();
        }
        else {
            return ToolResponse();
        }
    }
    else {
        return ToolResponse();
    }

    update_tip();

    clear_silkscreen();

    ClipperLib::ClipperOffset ofs_pads;
    ClipperLib::Paths pads_expanded;
    ofs_pads.AddPaths(pads, ClipperLib::jtMiter, ClipperLib::etClosedPolygon);
    ofs_pads.Execute(pads_expanded, settings.expand_pad + .075_mm);

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
                line->width = .15_mm;
                line->layer = BoardLayers::TOP_SILKSCREEN;
                line->from = from;
                line->to = to;
                from = to;
            }
        }
    }

    return ToolResponse();
}
} // namespace horizon
