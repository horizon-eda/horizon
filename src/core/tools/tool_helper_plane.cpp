#include "tool_helper_plane.hpp"
#include "board/board.hpp"
#include "document/idocument_board.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {
void ToolHelperPlane::plane_init(Polygon &poly)
{
    plane = dynamic_cast<Plane *>(poly.usage.ptr);
    if (!plane)
        return;

    if (plane->fragments.size() == 0)
        plane = nullptr;

    if (plane)
        plane->clear();
}

bool ToolHelperPlane::plane_finish()
{
    if (plane) {
        auto brd = doc.b->get_board();
        imp->tool_bar_set_tip("updating plane…");
        const auto ret = imp->dialogs.update_plane(*brd, plane);
        brd->update_airwires(false, {plane->net->uuid});
        return ret;
    }
    return true;
}

} // namespace horizon
