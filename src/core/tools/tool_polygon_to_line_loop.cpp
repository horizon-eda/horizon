#include "tool_polygon_to_line_loop.hpp"
#include "common/polygon.hpp"
#include "common/line.hpp"
#include "common/arc.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>
#include <functional>
#include "document/idocument.hpp"

namespace horizon {

ToolPolygonToLineLoop::ToolPolygonToLineLoop(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolPolygonToLineLoop::can_begin()
{
    if (!(doc.r->has_object_type(ObjectType::LINE) && doc.r->has_object_type(ObjectType::POLYGON)))
        return false;
    return get_polygon();
}

Polygon *ToolPolygonToLineLoop::get_polygon()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::POLYGON_ARC_CENTER || it.type == ObjectType::POLYGON_EDGE
            || it.type == ObjectType::POLYGON_VERTEX) {
            return doc.r->get_polygon(it.uuid);
        }
    }
    return nullptr;
}

Junction *ToolPolygonToLineLoop::make_junction(const Coordi &c)
{
    auto uu = UUID::random();
    auto ju = doc.r->insert_junction(uu);
    ju->position = c;
    return ju;
}

ToolResponse ToolPolygonToLineLoop::begin(const ToolArgs &args)
{
    auto poly = get_polygon();
    if (!poly) {
        return ToolResponse::revert();
    }

    const Polygon::Vertex *v0 = &poly->vertices.back();
    Junction *j0 = make_junction(v0->position);
    Junction *ju_back = j0;
    for (const auto &v : poly->vertices) {
        Junction *ju;
        if (&v != &poly->vertices.back()) {
            ju = make_junction(v.position);
        }
        else {
            ju = ju_back;
        }
        if (v0->type == Polygon::Vertex::Type::LINE) {
            auto li = doc.r->insert_line(UUID::random());
            li->from = j0;
            li->to = ju;
            li->width = 0;
            li->layer = poly->layer;
        }
        else if (v0->type == Polygon::Vertex::Type::ARC) {
            auto arc = doc.r->insert_arc(UUID::random());
            if (!v0->arc_reverse) {
                arc->from = j0;
                arc->to = ju;
            }
            else {
                arc->from = ju;
                arc->to = j0;
            }
            arc->width = 0;
            arc->layer = poly->layer;
            arc->center = make_junction(v0->arc_center);
        }
        v0 = &v;
        j0 = ju;
    }
    doc.r->delete_polygon(poly->uuid);

    return ToolResponse::commit();
}

ToolResponse ToolPolygonToLineLoop::update(const ToolArgs &args)
{

    return ToolResponse();
}
} // namespace horizon
