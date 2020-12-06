#pragma once
#include <vector>

namespace horizon {
template <class Ret> struct vector_accumulator {
    typedef std::vector<Ret> result_type;
    template <typename T_iterator> result_type operator()(T_iterator first, T_iterator last) const
    {
        result_type vec;
        for (; first != last; ++first)
            vec.push_back(*first);
        return vec;
    }
};

} // namespace horizon
