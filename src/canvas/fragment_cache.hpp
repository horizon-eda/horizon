#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"

namespace horizon {

class FragmentCache {
public:
    const std::vector<std::array<Coordf, 3>> &get_triangles(const class Plane &plane);

private:
    class CacheItem {
    public:
        unsigned int revision = 32768; // something not 0
        std::vector<std::array<Coordf, 3>> triangles;
    };
    std::map<UUID, CacheItem> planes;
};
} // namespace horizon
