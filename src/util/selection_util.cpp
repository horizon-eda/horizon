#include "selection_util.hpp"
#include <assert.h>

namespace horizon {
size_t sel_count_type(const std::set<SelectableRef> &sel, ObjectType type)
{
    return std::count_if(sel.begin(), sel.end(), [type](const auto &a) { return a.type == type; });
}

const SelectableRef &sel_find_one(const std::set<SelectableRef> &sel, ObjectType type)
{
    auto r = std::find_if(sel.begin(), sel.end(), [type](const auto &a) { return a.type == type; });
    if (r == sel.end())
        throw std::runtime_error("selectable not found");
    return *r;
}

template <class T, class Comp, class Alloc, class Predicate>
void discard_if(std::set<T, Comp, Alloc> &c, Predicate pred)
{
    for (auto it{c.begin()}, end{c.end()}; it != end;) {
        if (pred(*it)) {
            it = c.erase(it);
        }
        else {
            ++it;
        }
    }
}

void sel_erase_type(std::set<SelectableRef> &sel, ObjectType type)
{
    discard_if(sel, [type](const auto &a) { return a.type == type; });
}

} // namespace horizon
