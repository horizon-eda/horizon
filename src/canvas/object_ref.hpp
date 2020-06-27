#pragma once
#include "common/common.hpp"
#include "util/uuid.hpp"
#include <functional>

namespace horizon {
class ObjectRef {
public:
    ObjectRef(ObjectType ty, const UUID &uu, const UUID &uu2 = UUID()) : type(ty), uuid(uu), uuid2(uu2)
    {
    }
    ObjectRef() : type(ObjectType::INVALID)
    {
    }
    ObjectType type;
    UUID uuid;
    UUID uuid2;
    bool operator<(const ObjectRef &other) const
    {
        if (type < other.type) {
            return true;
        }
        if (type > other.type) {
            return false;
        }
        if (uuid < other.uuid) {
            return true;
        }
        else if (uuid > other.uuid) {
            return false;
        }
        return uuid2 < other.uuid2;
    }
    bool operator==(const ObjectRef &other) const
    {
        return (type == other.type) && (uuid == other.uuid) && (uuid2 == other.uuid2);
    }
    bool operator!=(const ObjectRef &other) const
    {
        return !(*this == other);
    }
};
} // namespace horizon

namespace std {
template <> struct hash<horizon::ObjectRef> {
    std::size_t operator()(const horizon::ObjectRef &k) const
    {
        return static_cast<size_t>(k.type) ^ std::hash<horizon::UUID>{}(k.uuid) ^ std::hash<horizon::UUID>{}(k.uuid2);
    }
};
} // namespace std
