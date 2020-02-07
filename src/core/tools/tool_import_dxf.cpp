#include "tool_import_dxf.hpp"
#include "document/idocument.hpp"
#include "imp/imp_interface.hpp"
#include "import_dxf/dxf_importer.hpp"
#include "common/junction.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include <gdk/gdkkeysyms.h>
#include <iostream>

namespace horizon {

ToolImportDXF::ToolImportDXF(IDocument *c, ToolID tid) : ToolBase(c, tid), ToolHelperMove(c, tid)
{
}

bool ToolImportDXF::can_begin()
{
    return doc.r->has_object_type(ObjectType::ARC) && doc.r->has_object_type(ObjectType::LINE);
}

ToolResponse ToolImportDXF::begin(const ToolArgs &args)
{
    bool r;
    std::string filename;

    std::tie(r, filename) = imp->dialogs.ask_dxf_filename();
    if (!r) {
        return ToolResponse::end();
    }

    DXFImporter importer(doc.r);
    importer.set_layer(args.work_layer);
    importer.set_scale(1);
    importer.set_shift(args.coords);
    importer.set_width(0);
    if (importer.import(filename)) {
        auto unsupported = importer.get_items_unsupported();
        selection.clear();
        for (const auto it : importer.junctions) {
            selection.emplace(it->uuid, ObjectType::JUNCTION);
        }
        lines = importer.lines;
        arcs = importer.arcs;
        move_init(args.coords);
        std::string s = "<b>LMB:</b>place <b>RMB:</b>cancel <b>r:</b>rotate <b>e:</b>mirror <b>w:</b>line width ";
        if (unsupported.size()) {
            s += "<span color='red' weight='bold'> Unsupported items: ";
            for (const auto &it : unsupported) {
                s += std::to_string(it.second) + " ";
                switch (it.first) {
                case DXFImporter::UnsupportedType::CIRCLE:
                    s += "Circle";
                    break;
                case DXFImporter::UnsupportedType::ELLIPSE:
                    s += "Ellipse";
                    break;
                case DXFImporter::UnsupportedType::SPLINE:
                    s += "Spline";
                    break;
                }
                if (it.second > 1)
                    s += "s";
                s += ", ";
            }
            s.pop_back();
            s.pop_back();
            s += "</span>";
        }
        imp->tool_bar_set_tip(s);
        return ToolResponse();
    }
    else {
        return ToolResponse::revert();
    }
    return ToolResponse::end();
}

ToolResponse ToolImportDXF::update(const ToolArgs &args)
{
    if (args.type == ToolEventType::MOVE) {
        move_do_cursor(args.coords);
    }
    else if (args.type == ToolEventType::CLICK) {
        if (args.button == 1) {
            return ToolResponse::commit();
        }
        else if (args.button == 3) {
            return ToolResponse::revert();
        }
    }
    else if (args.type == ToolEventType::KEY) {
        if (args.key == GDK_KEY_r) {
            move_mirror_or_rotate(args.coords, true);
        }
        else if (args.key == GDK_KEY_e) {
            move_mirror_or_rotate(args.coords, false);
        }
        else if (args.key == GDK_KEY_w) {
            auto r = imp->dialogs.ask_datum("Enter width", width);
            if (r.first) {
                width = r.second;
                for (auto it : lines) {
                    it->width = width;
                }
                for (auto it : arcs) {
                    it->width = width;
                }
            }
        }
    }
    else if (args.type == ToolEventType::LAYER_CHANGE) {
        for (auto it : lines) {
            it->layer = args.work_layer;
        }
        for (auto it : arcs) {
            it->layer = args.work_layer;
        }
    }
    return ToolResponse();
}
} // namespace horizon
