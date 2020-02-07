#include "tool_generate_courtyard.hpp"
#include "document/idocument_package.hpp"
#include "pool/package.hpp"
#include "board/board_layers.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolGenerateCourtyard::ToolGenerateCourtyard(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}


bool ToolGenerateCourtyard::can_begin()
{
    return doc.k;
}

ToolResponse ToolGenerateCourtyard::begin(const ToolArgs &args)
{
    Coordi a, b;
    auto pkg = doc.k->get_package();
    for (const auto &it : pkg->pads) {
        auto bb_pad = it.second.placement.transform_bb(it.second.padstack.get_bbox(true));
        a = Coordi::min(a, bb_pad.first);
        b = Coordi::max(b, bb_pad.second);
    }
    for (const auto &it_poly : pkg->polygons) {
        if (it_poly.second.layer == BoardLayers::TOP_PACKAGE) {
            auto poly = it_poly.second.remove_arcs();
            for (const auto &it : poly.vertices) {
                a = Coordi::min(a, it.position);
                b = Coordi::max(b, it.position);
            }
        }
    }

    if (a.mag_sq() == 0 || b.mag_sq() == 0) {
        imp->tool_bar_flash("zero courtyard, aborting");
        return ToolResponse::end();
    }

    Polygon *courtyard_poly = nullptr;
    for (auto &it : pkg->polygons) {
        if (it.second.layer == BoardLayers::TOP_COURTYARD) {
            courtyard_poly = &it.second;
            imp->tool_bar_flash("modified existing courtyard polygon");
            break;
        }
    }
    if (!courtyard_poly) {
        courtyard_poly = doc.k->insert_polygon(UUID::random());
        courtyard_poly->layer = BoardLayers::TOP_COURTYARD;
        imp->tool_bar_flash("created new courtyard polygon");
    }
    courtyard_poly->parameter_class = "courtyard";
    courtyard_poly->vertices.clear();
    courtyard_poly->vertices.emplace_back(a);
    courtyard_poly->vertices.emplace_back(Coordi(a.x, b.y));
    courtyard_poly->vertices.emplace_back(b);
    courtyard_poly->vertices.emplace_back(Coordi(b.x, a.y));

    return ToolResponse::commit();
}

ToolResponse ToolGenerateCourtyard::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
