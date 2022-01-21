#include "selection_util.hpp"
#include <assert.h>
#include <stdexcept>

namespace horizon {
size_t sel_count_type(const std::set<SelectableRef> &sel, ObjectType type)
{
    return std::count_if(sel.begin(), sel.end(), [type](const auto &a) { return a.type == type; });
}

bool sel_has_type(const std::set<SelectableRef> &sel, ObjectType type)
{
    auto r = std::find_if(sel.begin(), sel.end(), [type](const auto &a) { return a.type == type; });
    return r != sel.end();
}

const SelectableRef &sel_find_one(const std::set<SelectableRef> &sel, ObjectType type)
{
    auto r = std::find_if(sel.begin(), sel.end(), [type](const auto &a) { return a.type == type; });
    if (r == sel.end())
        throw std::runtime_error("selectable not found");
    return *r;
}

const SelectableRef *sel_find_exactly_one(const std::set<SelectableRef> &sel, ObjectType type)
{
    const SelectableRef *s = nullptr;
    for (const auto &it : sel) {
        if (it.type == type) {
            if (s == nullptr)
                s = &it;
            else
                return nullptr;
        }
    }
    return s;
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
