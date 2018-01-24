#include "tool_import_dxf.hpp"
#include "core.hpp"
#include "imp/imp_interface.hpp"
#include "import_dxf/dxf_importer.hpp"
#include <iostream>

namespace horizon {

ToolImportDXF::ToolImportDXF(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolImportDXF::can_begin()
{
    return core.r->has_object_type(ObjectType::ARC) && core.r->has_object_type(ObjectType::LINE);
}

ToolResponse ToolImportDXF::begin(const ToolArgs &args)
{
    bool r;
    std::string filename;
    int layer;
    int64_t line_width;
    double scale;

    std::tie(r, filename, layer, line_width, scale) = imp->dialogs.ask_dxf_filename(core.r);
    if (!r) {
        return ToolResponse::end();
    }

    DXFImporter importer(core.r);
    importer.set_layer(layer);
    importer.set_scale(scale);
    importer.set_shift(args.coords);
    importer.set_width(line_width);
    if (importer.import(filename)) {
        core.r->selection.clear();
        for (const auto it : importer.junctions) {
            core.r->selection.emplace(it->uuid, ObjectType::JUNCTION);
        }
        core.r->commit();
        return ToolResponse::next(ToolID::MOVE);
    }
    else {
        core.r->revert();
    }
    return ToolResponse::end();
}

ToolResponse ToolImportDXF::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
