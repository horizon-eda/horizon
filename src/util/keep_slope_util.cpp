#include "keep_slope_util.hpp"

namespace horizon {
KeepSlopeInfo::Position KeepSlopeInfo::get_pos(const Coordd &shift) const
{
    const Coordd vfrom = pos_from_orig - pos_from2;
    const Coordd vto = pos_to_orig - pos_to2;
    const Coordd vtr = pos_to_orig - pos_from_orig;
    const Coordd vtrn = Coordd(vtr.y, -vtr.x).normalize();

    const auto cos_alpha = vto.normalize().dot(vfrom.normalize());
    const auto n = 2. / (1 + cos_alpha); // 1/cos(α/2)²

    // shift projected onto vector perpendicular to track
    const Coordd vshift2 = vtrn * vtrn.dot(shift);

    Coordd shift_from = (vfrom * vshift2.mag_sq()) / (vfrom.dot(vshift2));
    Coordd shift_to = (vto * vshift2.mag_sq()) / (vto.dot(vshift2));
    if (vshift2.mag_sq() == 0) {
        shift_from = {0, 0};
        shift_to = {0, 0};
    }

    return {pos_from_orig + shift_from.to_coordi(), pos_to_orig + shift_to.to_coordi(), (vshift2 * n).to_coordi()};
}
} // namespace horizon
