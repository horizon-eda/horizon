#include "tool_draw_plane.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "core/tool_id.hpp"
#include "common/keepout.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolDrawPlane::can_begin()
{
    if (!ToolDrawPolygon::can_begin())
        return false;

    if (tool_id == ToolID::DRAW_KEEPOUT)
        return doc.r->has_object_type(ObjectType::KEEPOUT);
    else
        return doc.b;
}

ToolResponse ToolDrawPlane::commit()
{
    imp->canvas_update();
    if (tool_id == ToolID::DRAW_KEEPOUT) {
        auto keepout = doc.r->insert_keepout(UUID::random());
        keepout->polygon = temp;
        temp->usage = keepout;
        if (imp->dialogs.edit_keepout(*keepout, *doc.r, true))
            return ToolResponse::commit();
        else
            return ToolResponse::revert();
    }
    else {
        auto uu = UUID::random();
        auto &brd = *doc.b->get_board();
        auto &plane = brd.planes.emplace(uu, uu).first->second;
        plane.polygon = temp;
        temp->usage = &plane;
        if (imp->dialogs.edit_plane(plane, brd)) {
            brd.update_plane(&plane);
            return ToolResponse::commit();
        }
        else {
            return ToolResponse::revert();
        }
    }
    return ToolResponse::commit();
}
} // namespace horizon