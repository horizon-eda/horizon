#include "keep_slope_util.hpp"

namespace horizon {
std::pair<Coordi, Coordi> KeepSlopeInfo::get_pos(const Coordd &shift) const
{
    Coordd vfrom = pos_from_orig - pos_from2;
    Coordd vto = pos_to_orig - pos_to2;
    Coordd vtr = pos_to_orig - pos_from_orig;
    Coordd vtrn(vtr.y, -vtr.x);

    Coordd vshift2 = (vtrn * (vtrn.dot(shift))) / vtrn.mag_sq();

    Coordd shift_from = (vfrom * vshift2.mag_sq()) / (vfrom.dot(vshift2));
    Coordd shift_to = (vto * vshift2.mag_sq()) / (vto.dot(vshift2));
    if (vshift2.mag_sq() == 0) {
        shift_from = {0, 0};
        shift_to = {0, 0};
    }

    return {pos_from_orig + Coordi(shift_from.x, shift_from.y), pos_to_orig + Coordi(shift_to.x, shift_to.y)};
}
} // namespace horizon