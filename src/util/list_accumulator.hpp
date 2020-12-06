#pragma once
#include <list>

namespace horizon {
template <class Ret, bool back> struct list_accumulator {
    typedef std::list<Ret> result_type;
    template <typename T_iterator> result_type operator()(T_iterator first, T_iterator last) const
    {
        result_type lst;
        for (; first != last; ++first) {
            if (back)
                lst.push_back(*first);
            else
                lst.push_front(*first);
        }
        return lst;
    }
};

} // namespace horizon
