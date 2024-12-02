#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"

namespace horizon {

class PolygonCache {
public:
    const std::vector<std::array<Coordf, 3>> &get_triangles(const class Polygon &poly);

private:
    std::map<std::array<uint8_t, 20>, std::vector<std::array<Coordf, 3>>> cache;
};
} // namespace horizon
