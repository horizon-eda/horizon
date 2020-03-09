#pragma once
#include "common/common.hpp"
#include "canvas/selectables.hpp"
#include <set>

namespace horizon {
size_t sel_count_type(const std::set<SelectableRef> &sel, ObjectType type);
const SelectableRef &sel_find_one(const std::set<SelectableRef> &sel, ObjectType type);
void sel_erase_type(std::set<SelectableRef> &sel, ObjectType type);
} // namespace horizon
