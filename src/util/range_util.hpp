#pragma once
#include "common/common.hpp"
#include "canvas/selectables.hpp"
#include <range/v3/view.hpp>
#include <range/v3/algorithm.hpp>

namespace horizon {
inline auto sel_filter_type(ObjectType type)
{
    return ranges::views::filter([type](const SelectableRef &x) { return x.type == type; });
}

template <typename Tk, typename Tv> auto map_ptr_from_sel(std::map<Tk, Tv> &m)
{
    return [&m](const SelectableRef &x) { return &m.at(x.uuid); };
}


template <typename Tk, typename Tv> auto map_ref_from_sel(std::map<Tk, Tv> &m)
{
    return ranges::views::transform([&m](const SelectableRef &x) -> Tv & { return m.at(x.uuid); });
}

template <typename Tr, typename Tp> auto find_if_ptr(Tr &&range, Tp pred)
{
    auto x = ranges::find_if(range, pred);
    if (x == range.end())
        return static_cast<decltype(&*x)>(nullptr);
    else
        return &*x;
}
} // namespace horizon
