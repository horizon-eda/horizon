#include "action_icon.hpp"
#include "core/tool_id.hpp"
#include "actions.hpp"
#include <map>

namespace horizon {
static const std::map<ActionToolID, const char *> action_icons = {
        {make_action(ToolID::DRAW_POLYGON), "action-draw-polygon-symbolic"},
        {make_action(ToolID::DRAW_POLYGON_RECTANGLE), "action-draw-polygon-rectangle-symbolic"},
        {make_action(ToolID::DRAW_POLYGON_CIRCLE), "action-draw-polygon-circle-symbolic"},
        {make_action(ToolID::PLACE_TEXT), "action-place-text-symbolic"},
        {make_action(ToolID::PLACE_REFDES_AND_VALUE), "action-place-refdes-and-value-symbolic"},
        {make_action(ToolID::PLACE_SHAPE), "action-place-shape-circle-symbolic"},
        {make_action(ToolID::PLACE_SHAPE_OBROUND), "action-place-shape-obround-symbolic"},
        {make_action(ToolID::PLACE_SHAPE_RECTANGLE), "action-place-shape-rectangle-symbolic"},
        {make_action(ToolID::DRAW_LINE), "action-draw-line-symbolic"},
        {make_action(ToolID::DRAW_LINE_RECTANGLE), "action-draw-line-rectangle-symbolic"},
        {make_action(ToolID::DRAW_LINE_CIRCLE), "action-draw-line-circle-symbolic"},
        {make_action(ToolID::PLACE_HOLE), "action-place-hole-symbolic"},
        {make_action(ToolID::PLACE_BOARD_HOLE), "action-place-hole-symbolic"},
        {make_action(ToolID::PLACE_HOLE_SLOT), "action-place-hole-slot-symbolic"},
        {make_action(ToolID::RESIZE_SYMBOL), "action-resize-symbol-symbolic"},
        {make_action(ToolID::DRAW_ARC), "action-draw-arc-symbolic"},
        {make_action(ToolID::DRAW_DIMENSION), "action-draw-dimension-symbolic"},
        {make_action(ToolID::PLACE_PAD), "action-place-pad-symbolic"},
        {make_action(ToolID::PLACE_POWER_SYMBOL), "action-place-power-symbol-symbolic"},
        {make_action(ToolID::DRAW_NET), "action-draw-net-symbolic"},
        {make_action(ToolID::PLACE_NET_LABEL), "action-place-net-label-symbolic"},
        {make_action(ToolID::PLACE_BUS_LABEL), "action-place-bus-label-symbolic"},
        {make_action(ToolID::PLACE_BUS_RIPPER), "action-place-bus-ripper-symbolic"},
        {make_action(ActionID::PLACE_PART), "action-place-part-symbolic"},
        {make_action(ToolID::ROUTE_TRACK_INTERACTIVE), "action-route-track-symbolic"},
        {make_action(ToolID::ROUTE_DIFFPAIR_INTERACTIVE), "action-route-diffpair-symbolic"},
        {make_action(ToolID::PLACE_VIA), "action-place-via-symbolic"},
        {make_action(ToolID::PLACE_DOT), "action-place-dot-symbolic"},
};

const char *get_action_icon(ActionToolID id)
{
    if (action_icons.count(id))
        return action_icons.at(id);
    else
        return nullptr;
}
} // namespace horizon
