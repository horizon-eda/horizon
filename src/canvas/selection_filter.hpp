#pragma once
#include <map>
#include "common/common.hpp"

namespace horizon {
class SelectionFilter {
public:
    SelectionFilter(class Canvas *c) : ca(c)
    {
    }
    bool can_select(const class SelectableRef &sel) const;

    class ObjectFilter {
    public:
        std::map<int, bool> layers;
        bool other_layers = false;
    };

    std::map<ObjectType, ObjectFilter> object_filter;

private:
    Canvas *ca;
};
} // namespace horizon
