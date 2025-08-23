#pragma once
#include <string>
#include "common/common.hpp"
#include "util/placement.hpp"

namespace horizon {

template <typename T> std::pair<Coord<T>, Coord<T>> pad_bbox(std::pair<Coord<T>, Coord<T>> bb, T pad)
{
    bb.first.x -= pad;
    bb.first.y -= pad;

    bb.second.x += pad;
    bb.second.y += pad;
    return bb;
}
Coordd project_onto_perp_bisector(const Coordd &a, const Coordd &b, const Coordd &p);

std::string coord_to_string(const Coordf &c, bool delta = false);
std::string dim_to_string(int64_t x, bool with_sign = true);
std::string dim_to_string_nlz(int64_t x, bool with_sign = true);
std::string angle_to_string(int angle, bool pos_only = true);
int orientation_to_angle(Orientation o);
int64_t round_multiple(int64_t x, int64_t mul);

template <typename T> int sgn(T val)
{
    return (T(0) < val) - (val < T(0));
}

double angle_to_rad(int angle);
int angle_from_rad(double rad);
int wrap_angle(int angle);
template <typename T> T c2pi(T x);

Placement transform_package_placement_to_new_reference(Placement pl, Placement old_ref, Placement new_ref);
Placement transform_text_placement_to_new_reference(Placement pl, Placement old_ref, Placement new_ref);

} // namespace horizon
