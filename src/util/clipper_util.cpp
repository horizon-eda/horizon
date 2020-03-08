#include "clipper_util.hpp"
#include "placement.hpp"

namespace horizon {
static ClipperLib::IntPoint transform_intpt(const Placement &transform, const ClipperLib::IntPoint &pt)
{
    Coordi t(pt.X, pt.Y);
    auto t2 = transform.transform(t);
    return {t2.x, t2.y};
}

ClipperLib::Path transform_path(const Placement &transform, const ClipperLib::Path &path)
{
    ClipperLib::Path r;
    r.reserve(path.size());
    std::transform(path.begin(), path.end(), std::back_inserter(r),
                   [transform](const auto &x) { return transform_intpt(transform, x); });
    return r;
}
} // namespace horizon
