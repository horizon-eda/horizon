#pragma once
#include <map>
#include "common/common.hpp"

namespace horizon {
class SelectionFilter {
public:
    SelectionFilter(class Canvas *c) : ca(c)
    {
    }
    bool can_select(const class SelectableRef &sel);

    bool work_layer_only = false;
    std::map<ObjectType, bool> object_filter;

private:
    Canvas *ca;
};
} // namespace horizon
