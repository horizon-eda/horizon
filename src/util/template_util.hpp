#pragma once
#include <type_traits>

namespace horizon {
template <bool c, typename T> using make_const_ref_t = typename std::conditional<c, const T &, T &>::type;

}
