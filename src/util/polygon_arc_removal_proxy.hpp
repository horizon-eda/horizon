#pragma once
#include "common/polygon.hpp"
#include <optional>

namespace horizon {
class PolygonArcRemovalProxy {
public:
    PolygonArcRemovalProxy(const Polygon &poly, unsigned int precision = 16);
    const Polygon &get() const;
    bool had_arcs() const;

private:
    const Polygon &parent;
    std::optional<Polygon> poly_arcs_removed;
    const Polygon *ppoly = nullptr;
};
} // namespace horizon
