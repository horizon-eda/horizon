#include "polygon_arc_removal_proxy.hpp"

namespace horizon {

PolygonArcRemovalProxy::PolygonArcRemovalProxy(const Polygon &poly, unsigned int precision) : parent(poly)
{
    if (parent.has_arcs()) {
        poly_arcs_removed.emplace(parent.remove_arcs(precision));
        ppoly = &poly_arcs_removed.value();
    }
    else {
        ppoly = &parent;
    }
}

const Polygon &PolygonArcRemovalProxy::get() const
{
    return *ppoly;
}

bool PolygonArcRemovalProxy::had_arcs() const
{
    return ppoly != &parent;
}

} // namespace horizon
