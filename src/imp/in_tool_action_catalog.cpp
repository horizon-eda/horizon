#include "in_tool_action_catalog.hpp"
#include "core/tool_id.hpp"

namespace horizon {

const std::map<InToolActionID, InToolActionCatalogItem> in_tool_action_catalog = {
        {InToolActionID::LMB, {"place", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},
        {InToolActionID::RMB, {"cancel", ToolID::NONE, InToolActionCatalogItem::FLAGS_NO_PREFERENCES}},

        {InToolActionID::ROTATE, {"rotate", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ROTATE_CURSOR, {"rotate around cursor", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::MIRROR, {"mirror", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::MIRROR_CURSOR, {"mirror around cursor", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::RESTRICT, {"restrict", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::POSTURE, {"posture", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ENTER_WIDTH, {"enter width", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ENTER_SIZE, {"enter size", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ENTER_DATUM, {"enter datum", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::EDIT, {"edit", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::RECTANGLE_MODE, {"rectangle mode", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::NET_LABEL_SIZE_INC,
         {"increase net label size", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::NET_LABEL_SIZE_DEC,
         {"decrease net label size", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::FLIP_ARC, {"flip arc", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::TOGGLE_ARC, {"toggle arc", ToolID::DRAW_POLYGON, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::DIMENSION_MODE,
         {"dimension mode", ToolID::DRAW_DIMENSION, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::PLACE_JUNCTION, {"place junction", ToolID::DRAW_NET, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ARBITRARY_ANGLE_MODE,
         {"arbitrary angle", ToolID::DRAW_NET, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_NET_LABEL,
         {"toggle net label", ToolID::DRAW_NET, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::POLYGON_DECORATION_POSITION,
         {"decoration position", ToolID::DRAW_POLYGON_RECTANGLE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::POLYGON_DECORATION_SIZE,
         {"decoration size", ToolID::DRAW_POLYGON_RECTANGLE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::POLYGON_DECORATION_STYLE,
         {"decoration style", ToolID::DRAW_POLYGON_RECTANGLE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::POLYGON_CORNER_RADIUS,
         {"corner radius", ToolID::DRAW_POLYGON_RECTANGLE, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::AUTOPLACE_NEXT_PIN,
         {"autoplace next", ToolID::MAP_PIN, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::AUTOPLACE_ALL_PINS,
         {"autoplace all", ToolID::MAP_PIN, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::TOGGLE_ANGLE_SNAP,
         {"toggle angle snap", ToolID::ROTATE_ARBITRARY, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::LENGTH_TUNING_LENGTH,
         {"target length", ToolID::TUNE_TRACK, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::LENGTH_TUNING_AMPLITUDE_INC,
         {"increase meander amplitude", ToolID::TUNE_TRACK, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::LENGTH_TUNING_SPACING_DEC,
         {"decrease meander spacing", ToolID::TUNE_TRACK, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::LENGTH_TUNING_SPACING_INC,
         {"increase meander spacing", ToolID::TUNE_TRACK, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::LENGTH_TUNING_AMPLITUDE_DEC,
         {"decrease meander amplitude", ToolID::TUNE_TRACK, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::TOGGLE_VIA,
         {"toggle via", ToolID::ROUTE_TRACK_INTERACTIVE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TRACK_WIDTH_DEFAULT,
         {"default track width", ToolID::ROUTE_TRACK_INTERACTIVE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ROUTER_SETTINGS,
         {"settings", ToolID::ROUTE_TRACK_INTERACTIVE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::CLEARANCE_OFFSET,
         {"clearance offset", ToolID::ROUTE_TRACK_INTERACTIVE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::CLEARANCE_OFFSET_DEFAULT,
         {"default clearance offset", ToolID::ROUTE_TRACK_INTERACTIVE, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::ROUTER_MODE,
         {"shove/walkaround", ToolID::ROUTE_TRACK_INTERACTIVE, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::NC_MODE, {"NC mode", ToolID::SET_NC, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::FLIP_DIRECTION,
         {"Flip direction", ToolID::ADD_VERTEX, InToolActionCatalogItem::FLAGS_DEFAULT}},

        {InToolActionID::ARC_MODE, {"arc mode", ToolID::DRAW_ARC, InToolActionCatalogItem::FLAGS_DEFAULT}},
        {InToolActionID::TOGGLE_DEG45_RESTRICT, {"toggle 45 degree restrict", ToolID::NONE, InToolActionCatalogItem::FLAGS_DEFAULT}},
};


#define LUT_ITEM(x)                                                                                                    \
    {                                                                                                                  \
#x, InToolActionID::x                                                                                          \
    }

const LutEnumStr<InToolActionID> in_tool_action_lut = {
        LUT_ITEM(LMB),
        LUT_ITEM(LMB_RELEASE),
        LUT_ITEM(RMB),
        LUT_ITEM(ROTATE),
        LUT_ITEM(ROTATE_CURSOR),
        LUT_ITEM(MIRROR),
        LUT_ITEM(MIRROR_CURSOR),
        LUT_ITEM(CANCEL),
        LUT_ITEM(COMMIT),
        LUT_ITEM(RESTRICT),
        LUT_ITEM(POSTURE),
        LUT_ITEM(ENTER_WIDTH),
        LUT_ITEM(ENTER_SIZE),
        LUT_ITEM(ENTER_DATUM),
        LUT_ITEM(EDIT),
        LUT_ITEM(MOVE_UP),
        LUT_ITEM(MOVE_DOWN),
        LUT_ITEM(MOVE_LEFT),
        LUT_ITEM(MOVE_RIGHT),
        LUT_ITEM(MOVE_UP_FINE),
        LUT_ITEM(MOVE_DOWN_FINE),
        LUT_ITEM(MOVE_LEFT_FINE),
        LUT_ITEM(MOVE_RIGHT_FINE),
        LUT_ITEM(RECTANGLE_MODE),
        LUT_ITEM(NET_LABEL_SIZE_INC),
        LUT_ITEM(NET_LABEL_SIZE_DEC),
        LUT_ITEM(FLIP_ARC),
        LUT_ITEM(TOGGLE_ARC),
        LUT_ITEM(DIMENSION_MODE),
        LUT_ITEM(PLACE_JUNCTION),
        LUT_ITEM(ARBITRARY_ANGLE_MODE),
        LUT_ITEM(TOGGLE_NET_LABEL),
        LUT_ITEM(POLYGON_DECORATION_POSITION),
        LUT_ITEM(POLYGON_DECORATION_SIZE),
        LUT_ITEM(POLYGON_DECORATION_STYLE),
        LUT_ITEM(POLYGON_CORNER_RADIUS),
        LUT_ITEM(AUTOPLACE_NEXT_PIN),
        LUT_ITEM(AUTOPLACE_ALL_PINS),
        LUT_ITEM(TOGGLE_ANGLE_SNAP),
        LUT_ITEM(LENGTH_TUNING_LENGTH),
        LUT_ITEM(LENGTH_TUNING_AMPLITUDE_INC),
        LUT_ITEM(LENGTH_TUNING_AMPLITUDE_DEC),
        LUT_ITEM(LENGTH_TUNING_SPACING_INC),
        LUT_ITEM(LENGTH_TUNING_SPACING_DEC),
        LUT_ITEM(TOGGLE_VIA),
        LUT_ITEM(TRACK_WIDTH_DEFAULT),
        LUT_ITEM(ROUTER_SETTINGS),
        LUT_ITEM(CLEARANCE_OFFSET),
        LUT_ITEM(CLEARANCE_OFFSET_DEFAULT),
        LUT_ITEM(ROUTER_MODE),
        LUT_ITEM(NC_MODE),
        LUT_ITEM(FLIP_DIRECTION),
        LUT_ITEM(ARC_MODE),
        LUT_ITEM(TOGGLE_DEG45_RESTRICT),
};


} // namespace horizon
