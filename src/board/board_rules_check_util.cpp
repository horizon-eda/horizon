#include "board_rules_check_util.hpp"
#include "block/net.hpp"
#include "util/util.hpp"
#include <sstream>

namespace horizon {
std::string get_net_name(const Net *net)
{
    if (!net || net->name.size() == 0)
        return "";
    else
        return "(" + net->name + ")";
}

ClipperLib::IntRect get_patch_bb(const ClipperLib::Paths &patch)
{
    auto p = patch.front().front();
    ClipperLib::IntRect rect;
    rect.left = rect.right = p.X;
    rect.top = rect.bottom = p.Y;
    for (const auto &path : patch) {
        for (const auto &pt : path) {
            rect.left = std::min(rect.left, pt.X);
            rect.bottom = std::min(rect.bottom, pt.Y);
            rect.right = std::max(rect.right, pt.X);
            rect.top = std::max(rect.top, pt.Y);
        }
    }
    return rect;
}

bool bbox_test_overlap(const ClipperLib::IntRect &bb1, const ClipperLib::IntRect &bb2, uint64_t clearance)
{
    const int64_t offset = clearance + 10; // just to be safe
    return (((bb1.right + offset) >= bb2.left && bb2.right >= (bb1.left - offset))
            && ((bb1.top + offset) >= bb2.bottom && bb2.top >= (bb1.bottom - offset)));
}

void format_progress(std::ostringstream &oss, size_t i, size_t n)
{
    const unsigned int percentage = (i * 100) / n;
    oss << format_m_of_n(i, n) << "  ";
    if (percentage < 10)
        oss << " ";
    oss << percentage << "%";
}

} // namespace horizon
