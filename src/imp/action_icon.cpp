#include "action_icon.hpp"
#include "core/tool_id.hpp"
#include "actions.hpp"
#include <map>

namespace horizon {
static const std::map<ActionToolID, const char *> action_icons = {
        {ToolID::DRAW_POLYGON, "action-draw-polygon-symbolic"},
        {ToolID::DRAW_POLYGON_RECTANGLE, "action-draw-polygon-rectangle-symbolic"},
        {ToolID::DRAW_POLYGON_CIRCLE, "action-draw-polygon-circle-symbolic"},
        {ToolID::PLACE_TEXT, "action-place-text-symbolic"},
        {ToolID::PLACE_REFDES_AND_VALUE, "action-place-refdes-and-value-symbolic"},
        {ToolID::PLACE_SHAPE, "action-place-shape-circle-symbolic"},
        {ToolID::PLACE_SHAPE_OBROUND, "action-place-shape-obround-symbolic"},
        {ToolID::PLACE_SHAPE_RECTANGLE, "action-place-shape-rectangle-symbolic"},
        {ToolID::DRAW_LINE, "action-draw-line-symbolic"},
        {ToolID::DRAW_LINE_RECTANGLE, "action-draw-line-rectangle-symbolic"},
        {ToolID::DRAW_LINE_CIRCLE, "action-draw-line-circle-symbolic"},
        {ToolID::PLACE_HOLE, "action-place-hole-symbolic"},
        {ToolID::PLACE_BOARD_HOLE, "action-place-hole-symbolic"},
        {ToolID::PLACE_HOLE_SLOT, "action-place-hole-slot-symbolic"},
        {ToolID::RESIZE_SYMBOL, "action-resize-symbol-symbolic"},
        {ToolID::DRAW_ARC, "action-draw-arc-symbolic"},
        {ToolID::DRAW_DIMENSION, "action-draw-dimension-symbolic"},
        {ToolID::PLACE_PAD, "action-place-pad-symbolic"},
        {ToolID::PLACE_POWER_SYMBOL, "action-place-power-symbol-symbolic"},
        {ToolID::DRAW_NET, "action-draw-net-symbolic"},
        {ToolID::PLACE_NET_LABEL, "action-place-net-label-symbolic"},
        {ToolID::PLACE_BUS_LABEL, "action-place-bus-label-symbolic"},
        {ToolID::PLACE_BUS_RIPPER, "action-place-bus-ripper-symbolic"},
        {ActionID::PLACE_PART, "action-place-part-symbolic"},
        {ToolID::ROUTE_TRACK_INTERACTIVE, "action-route-track-symbolic"},
        {ToolID::ROUTE_DIFFPAIR_INTERACTIVE, "action-route-diffpair-symbolic"},
        {ToolID::PLACE_VIA, "action-place-via-symbolic"},
        {ToolID::PLACE_DOT, "action-place-dot-symbolic"},
};

const char *get_action_icon(ActionToolID id)
{
    if (action_icons.count(id))
        return action_icons.at(id);
    else
        return nullptr;
}
} // namespace horizon
