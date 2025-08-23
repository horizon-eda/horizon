#include "tool_draw_plane.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "core/tool_id.hpp"
#include "common/keepout.hpp"
#include "imp/imp_interface.hpp"
#include "core/tool_data_window.hpp"

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
    done = true;
    imp->canvas_update();
    imp->tool_bar_set_actions({});
    imp->tool_bar_set_tip("");
    if (tool_id == ToolID::DRAW_KEEPOUT) {
        auto keepout = doc.r->insert_keepout(UUID::random());
        keepout->polygon = temp;
        temp->usage = keepout;
        if (imp->dialogs.edit_keepout(*keepout, *doc.r, true))
            return ToolResponse::commit();
        else
            return ToolResponse::revert();
    }
    else if (tool_id == ToolID::DRAW_PLANE) {
        auto uu = UUID::random();
        auto &brd = *doc.b->get_board();
        auto &plane = brd.planes.emplace(uu, uu).first->second;
        plane.polygon = temp;
        temp->usage = &plane;

        show_edit_plane_window(plane, brd);
        return ToolResponse();
    }
    else if (tool_id == ToolID::DRAW_HEIGHT_RESTRICTION) {
        auto uu = UUID::random();
        auto &brd = *doc.b->get_board();
        auto &hr = brd.height_restrictions.emplace(uu, uu).first->second;
        hr.polygon = temp;
        temp->usage = &hr;

        return ToolResponse::commit();
    }
    else {
        return ToolResponse::end();
    }
    return ToolResponse::commit();
}

ToolResponse ToolDrawPlane::update(const ToolArgs &args)
{
    if (done) {
        return update_for_plane(args);
    }
    else {
        return ToolDrawPolygon::update(args);
    }
    return ToolResponse();
}


} // namespace horizon
