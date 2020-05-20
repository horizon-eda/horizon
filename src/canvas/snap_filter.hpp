#pragma once
#include "common/common.hpp"
#include "util/uuid_path.hpp"

namespace horizon {
class SnapFilter {
public:
    UUID uu;
    ObjectType type;
    int vertex = 0;
    SnapFilter(ObjectType ot, const UUID &u, int v = -1) : uu(u), type(ot), vertex(v){};
    bool operator<(const SnapFilter &other) const
    {
        if (type < other.type) {
            return true;
        }
        if (type > other.type) {
            return false;
        }
        if (uu < other.uu) {
            return true;
        }
        else if (other.uu < uu) {
            return false;
        }
        return vertex < other.vertex;
    }
    bool operator==(const SnapFilter &other) const
    {
        return (uu == other.uu) && (vertex == other.vertex) && (type == other.type);
    }
};
} // namespace horizon
