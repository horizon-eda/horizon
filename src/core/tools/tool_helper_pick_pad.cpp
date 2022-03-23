#include "tool_helper_pick_pad.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {
std::optional<ToolHelperPickPad::PkgPad> ToolHelperPickPad::pad_from_target(const Target &target)
{
    if (!(target.type == ObjectType::PAD || target.type == ObjectType::BOARD_PACKAGE))
        return {};
    auto brd = doc.b->get_board();
    auto pkg = &brd->packages.at(target.path.at(0));

    Pad *pad = nullptr;
    if (target.type == ObjectType::BOARD_PACKAGE) {
        // If the selected package has a pin on the origin, it's possible for us to
        // select the package instead of the pad.
        for (auto &[pad_uuid, pkg_pad] : pkg->package.pads) {
            if (pkg_pad.placement.shift.x == 0 && pkg_pad.placement.shift.y == 0) {
                if (pad) {
                    imp->tool_bar_flash("multiple pads on package origin");
                    return {};
                }
                pad = &pkg_pad;
            }
        }
    }
    else {
        pad = &pkg->package.pads.at(target.path.at(1));
    }

    if (pad)
        return {{*pkg, *pad}};
    else
        return {};
}

} // namespace horizon
