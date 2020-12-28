#include "tool_helper_plane.hpp"
#include "board/board.hpp"
#include "document/idocument_board.hpp"

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

void ToolHelperPlane::plane_finish()
{
    if (plane) {
        auto brd = doc.b->get_board();
        brd->update_plane(plane);
        brd->update_airwires(false, {plane->net->uuid});
    }
}

} // namespace horizon
