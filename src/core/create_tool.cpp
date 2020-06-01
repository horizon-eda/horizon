#include "core.hpp"
#include "tool_id.hpp"
#include "tools/tool_add_part.hpp"
#include "tools/tool_add_vertex.hpp"
#include "tools/tool_assign_part.hpp"
#include "tools/tool_bend_line_net.hpp"
#include "tools/tool_delete.hpp"
#include "tools/tool_disconnect.hpp"
#include "tools/tool_drag_keep_slope.hpp"
#include "tools/tool_draw_arc.hpp"
#include "tools/tool_draw_dimension.hpp"
#include "tools/tool_draw_line.hpp"
#include "tools/tool_draw_line_net.hpp"
#include "tools/tool_draw_line_rectangle.hpp"
#include "tools/tool_draw_polygon.hpp"
#include "tools/tool_draw_polygon_rectangle.hpp"
#include "tools/tool_draw_track.hpp"
#include "tools/tool_edit_board_hole.hpp"
#include "tools/tool_edit_line_rectangle.hpp"
#include "tools/tool_edit_pad_parameter_set.hpp"
#include "tools/tool_edit_plane.hpp"
#include "tools/tool_edit_shape.hpp"
#include "tools/tool_edit_symbol_pin_names.hpp"
#include "tools/tool_edit_via.hpp"
#include "tools/tool_enter_datum.hpp"
#include "tools/tool_import_dxf.hpp"
#include "tools/tool_lock.hpp"
#include "tools/tool_manage_buses.hpp"
#include "tools/tool_map_package.hpp"
#include "tools/tool_map_pin.hpp"
#include "tools/tool_map_symbol.hpp"
#include "tools/tool_move.hpp"
#include "tools/tool_move_net_segment.hpp"
#include "tools/tool_paste.hpp"
#include "tools/tool_place_board_hole.hpp"
#include "tools/tool_place_bus_label.hpp"
#include "tools/tool_place_bus_ripper.hpp"
#include "tools/tool_place_hole.hpp"
#include "tools/tool_place_junction.hpp"
#include "tools/tool_place_net_label.hpp"
#include "tools/tool_place_pad.hpp"
#include "tools/tool_place_power_symbol.hpp"
#include "tools/tool_place_shape.hpp"
#include "tools/tool_place_text.hpp"
#include "tools/tool_place_via.hpp"
#include "tools/tool_rotate_arbitrary.hpp"
#include "tools/tool_route_track.hpp"
#include "tools/tool_route_track_interactive.hpp"
#include "tools/tool_set_diffpair.hpp"
#include "tools/tool_set_via_net.hpp"
#include "tools/tool_smash.hpp"
#include "tools/tool_update_all_planes.hpp"
#include "tools/tool_generate_courtyard.hpp"
#include "tools/tool_generate_silkscreen.hpp"
#include "tools/tool_set_group.hpp"
#include "tools/tool_copy_placement.hpp"
#include "tools/tool_copy_tracks.hpp"
#include "tools/tool_swap_nets.hpp"
#include "tools/tool_line_loop_to_polygon.hpp"
#include "tools/tool_change_unit.hpp"
#include "tools/tool_set_nc_all.hpp"
#include "tools/tool_set_nc.hpp"
#include "tools/tool_add_keepout.hpp"
#include "tools/tool_change_symbol.hpp"
#include "tools/tool_place_refdes_and_value.hpp"
#include "tools/tool_draw_polygon_circle.hpp"
#include "tools/tool_draw_connection_line.hpp"
#include "tools/tool_backannotate_connection_lines.hpp"
#include "tools/tool_import_kicad_package.hpp"
#include "tools/tool_smash_silkscreen_graphics.hpp"
#include "tools/tool_renumber_pads.hpp"
#include "tools/tool_fix.hpp"
#include "tools/tool_nopopulate.hpp"
#include "tools/tool_polygon_to_line_loop.hpp"
#include "tools/tool_place_board_panel.hpp"
#include "tools/tool_smash_panel_outline.hpp"
#include "tools/tool_smash_package_outline.hpp"
#include "tools/tool_resize_symbol.hpp"
#include "tools/tool_round_off_vertex.hpp"
#include "tools/tool_swap_gates.hpp"
#include "tools/tool_place_picture.hpp"

namespace horizon {

std::unique_ptr<ToolBase> Core::create_tool(ToolID tool_id)
{
    switch (tool_id) {
    case ToolID::MOVE:
    case ToolID::MOVE_EXACTLY:
    case ToolID::MIRROR_X:
    case ToolID::MIRROR_Y:
    case ToolID::ROTATE:
    case ToolID::MOVE_KEY:
    case ToolID::MOVE_KEY_UP:
    case ToolID::MOVE_KEY_DOWN:
    case ToolID::MOVE_KEY_LEFT:
    case ToolID::MOVE_KEY_RIGHT:
    case ToolID::MOVE_KEY_FINE_UP:
    case ToolID::MOVE_KEY_FINE_DOWN:
    case ToolID::MOVE_KEY_FINE_LEFT:
    case ToolID::MOVE_KEY_FINE_RIGHT:
    case ToolID::ROTATE_CURSOR:
    case ToolID::MIRROR_CURSOR:
        return std::make_unique<ToolMove>(this, tool_id);

    case ToolID::PLACE_JUNCTION:
        return std::make_unique<ToolPlaceJunction>(this, tool_id);

    case ToolID::DRAW_LINE:
        return std::make_unique<ToolDrawLine>(this, tool_id);

    case ToolID::DELETE:
        return std::make_unique<ToolDelete>(this, tool_id);

    case ToolID::DRAW_ARC:
        return std::make_unique<ToolDrawArc>(this, tool_id);

    case ToolID::MAP_PIN:
        return std::make_unique<ToolMapPin>(this, tool_id);

    case ToolID::MAP_SYMBOL:
        return std::make_unique<ToolMapSymbol>(this, tool_id);

    case ToolID::DRAW_NET:
        return std::make_unique<ToolDrawLineNet>(this, tool_id);

    case ToolID::ADD_COMPONENT:
    case ToolID::ADD_PART:
        return std::make_unique<ToolAddPart>(this, tool_id);

    case ToolID::PLACE_TEXT:
        return std::make_unique<ToolPlaceText>(this, tool_id);

    case ToolID::PLACE_NET_LABEL:
        return std::make_unique<ToolPlaceNetLabel>(this, tool_id);

    case ToolID::DISCONNECT:
        return std::make_unique<ToolDisconnect>(this, tool_id);

    case ToolID::BEND_LINE_NET:
        return std::make_unique<ToolBendLineNet>(this, tool_id);

    case ToolID::SELECT_NET_SEGMENT:
    case ToolID::MOVE_NET_SEGMENT:
    case ToolID::MOVE_NET_SEGMENT_NEW:
        return std::make_unique<ToolMoveNetSegment>(this, tool_id);

    case ToolID::PLACE_POWER_SYMBOL:
        return std::make_unique<ToolPlacePowerSymbol>(this, tool_id);

    case ToolID::EDIT_SYMBOL_PIN_NAMES:
        return std::make_unique<ToolEditSymbolPinNames>(this, tool_id);

    case ToolID::PLACE_BUS_LABEL:
        return std::make_unique<ToolPlaceBusLabel>(this, tool_id);

    case ToolID::PLACE_BUS_RIPPER:
        return std::make_unique<ToolPlaceBusRipper>(this, tool_id);

    case ToolID::MANAGE_BUSES:
    case ToolID::MANAGE_NET_CLASSES:
    case ToolID::EDIT_STACKUP:
    case ToolID::ANNOTATE:
    case ToolID::EDIT_SCHEMATIC_PROPERTIES:
    case ToolID::MANAGE_POWER_NETS:
    case ToolID::EDIT_FRAME_PROPERTIES:
    case ToolID::TOGGLE_GROUP_TAG_VISIBLE:
    case ToolID::MANAGE_INCLUDED_BOARDS:
        return std::make_unique<ToolManageBuses>(this, tool_id);

    case ToolID::DRAW_POLYGON:
        return std::make_unique<ToolDrawPolygon>(this, tool_id);

    case ToolID::ENTER_DATUM:
        return std::make_unique<ToolEnterDatum>(this, tool_id);

    case ToolID::PLACE_HOLE:
    case ToolID::PLACE_HOLE_SLOT:
        return std::make_unique<ToolPlaceHole>(this, tool_id);

    case ToolID::PLACE_PAD:
        return std::make_unique<ToolPlacePad>(this, tool_id);

    case ToolID::PASTE:
    case ToolID::DUPLICATE:
        return std::make_unique<ToolPaste>(this, tool_id);

    case ToolID::ASSIGN_PART:
        return std::make_unique<ToolAssignPart>(this, tool_id);

    case ToolID::MAP_PACKAGE:
        return std::make_unique<ToolMapPackage>(this, tool_id);

    case ToolID::DRAW_TRACK:
        return std::make_unique<ToolDrawTrack>(this, tool_id);

    case ToolID::PLACE_VIA:
        return std::make_unique<ToolPlaceVia>(this, tool_id);

    case ToolID::ROUTE_TRACK:
        return std::make_unique<ToolRouteTrack>(this, tool_id);

    case ToolID::DRAG_KEEP_SLOPE:
        return std::make_unique<ToolDragKeepSlope>(this, tool_id);

    case ToolID::SMASH:
    case ToolID::UNSMASH:
        return std::make_unique<ToolSmash>(this, tool_id);

    case ToolID::PLACE_SHAPE:
    case ToolID::PLACE_SHAPE_OBROUND:
    case ToolID::PLACE_SHAPE_RECTANGLE:
        return std::make_unique<ToolPlaceShape>(this, tool_id);

    case ToolID::EDIT_SHAPE:
        return std::make_unique<ToolEditShape>(this, tool_id);

    case ToolID::IMPORT_DXF:
        return std::make_unique<ToolImportDXF>(this, tool_id);

    case ToolID::EDIT_PAD_PARAMETER_SET:
        return std::make_unique<ToolEditPadParameterSet>(this, tool_id);

    case ToolID::DRAW_POLYGON_RECTANGLE:
        return std::make_unique<ToolDrawPolygonRectangle>(this, tool_id);

    case ToolID::DRAW_LINE_RECTANGLE:
        return std::make_unique<ToolDrawLineRectangle>(this, tool_id);

    case ToolID::EDIT_LINE_RECTANGLE:
        return std::make_unique<ToolEditLineRectangle>(this, tool_id);

    case ToolID::ROUTE_TRACK_INTERACTIVE:
    case ToolID::ROUTE_DIFFPAIR_INTERACTIVE:
    case ToolID::DRAG_TRACK_INTERACTIVE:
    case ToolID::TUNE_TRACK:
    case ToolID::TUNE_DIFFPAIR:
    case ToolID::TUNE_DIFFPAIR_SKEW:
        return std::make_unique<ToolRouteTrackInteractive>(this, tool_id);

    case ToolID::EDIT_VIA:
        return std::make_unique<ToolEditVia>(this, tool_id);

    case ToolID::ROTATE_ARBITRARY:
    case ToolID::SCALE:
        return std::make_unique<ToolRotateArbitrary>(this, tool_id);

    case ToolID::ADD_PLANE:
    case ToolID::EDIT_PLANE:
    case ToolID::CLEAR_PLANE:
    case ToolID::UPDATE_PLANE:
        return std::make_unique<ToolEditPlane>(this, tool_id);

    case ToolID::CLEAR_ALL_PLANES:
    case ToolID::UPDATE_ALL_PLANES:
        return std::make_unique<ToolUpdateAllPlanes>(this, tool_id);

    case ToolID::DRAW_DIMENSION:
        return std::make_unique<ToolDrawDimension>(this, tool_id);

    case ToolID::SET_DIFFPAIR:
    case ToolID::CLEAR_DIFFPAIR:
        return std::make_unique<ToolSetDiffpair>(this, tool_id);

    case ToolID::SET_VIA_NET:
    case ToolID::CLEAR_VIA_NET:
        return std::make_unique<ToolSetViaNet>(this, tool_id);

    case ToolID::LOCK:
    case ToolID::UNLOCK:
    case ToolID::UNLOCK_ALL:
        return std::make_unique<ToolLock>(this, tool_id);

    case ToolID::ADD_VERTEX:
        return std::make_unique<ToolAddVertex>(this, tool_id);

    case ToolID::PLACE_BOARD_HOLE:
        return std::make_unique<ToolPlaceBoardHole>(this, tool_id);

    case ToolID::EDIT_BOARD_HOLE:
        return std::make_unique<ToolEditBoardHole>(this, tool_id);

    case ToolID::GENERATE_COURTYARD:
        return std::make_unique<ToolGenerateCourtyard>(this, tool_id);

    case ToolID::GENERATE_SILKSCREEN:
        return std::make_unique<ToolGenerateSilkscreen>(this, tool_id);

    case ToolID::SET_GROUP:
    case ToolID::SET_NEW_GROUP:
    case ToolID::CLEAR_GROUP:
    case ToolID::SET_TAG:
    case ToolID::SET_NEW_TAG:
    case ToolID::CLEAR_TAG:
    case ToolID::RENAME_GROUP:
    case ToolID::RENAME_TAG:
        return std::make_unique<ToolSetGroup>(this, tool_id);

    case ToolID::COPY_PLACEMENT:
        return std::make_unique<ToolCopyPlacement>(this, tool_id);

    case ToolID::COPY_TRACKS:
        return std::make_unique<ToolCopyTracks>(this, tool_id);

    case ToolID::SWAP_NETS:
        return std::make_unique<ToolSwapNets>(this, tool_id);

    case ToolID::LINE_LOOP_TO_POLYGON:
        return std::make_unique<ToolLineLoopToPolygon>(this, tool_id);

    case ToolID::CHANGE_UNIT:
        return std::make_unique<ToolChangeUnit>(this, tool_id);

    case ToolID::SET_ALL_NC:
    case ToolID::CLEAR_ALL_NC:
        return std::make_unique<ToolSetNotConnectedAll>(this, tool_id);

    case ToolID::SET_NC:
    case ToolID::CLEAR_NC:
        return std::make_unique<ToolSetNotConnected>(this, tool_id);

    case ToolID::ADD_KEEPOUT:
    case ToolID::EDIT_KEEPOUT:
        return std::make_unique<ToolAddKeepout>(this, tool_id);

    case ToolID::CHANGE_SYMBOL:
        return std::make_unique<ToolChangeSymbol>(this, tool_id);

    case ToolID::PLACE_REFDES_AND_VALUE:
        return std::make_unique<ToolPlaceRefdesAndValue>(this, tool_id);

    case ToolID::DRAW_POLYGON_CIRCLE:
        return std::make_unique<ToolDrawPolygonCircle>(this, tool_id);

    case ToolID::DRAW_CONNECTION_LINE:
        return std::make_unique<ToolDrawConnectionLine>(this, tool_id);

    case ToolID::BACKANNOTATE_CONNECTION_LINES:
        return std::make_unique<ToolBackannotateConnectionLines>(this, tool_id);

    case ToolID::IMPORT_KICAD_PACKAGE:
        return std::make_unique<ToolImportKiCadPackage>(this, tool_id);

    case ToolID::SMASH_SILKSCREEN_GRAPHICS:
        return std::make_unique<ToolSmashSilkscreenGraphics>(this, tool_id);

    case ToolID::RENUMBER_PADS:
        return std::make_unique<ToolRenumberPads>(this, tool_id);

    case ToolID::FIX:
    case ToolID::UNFIX:
        return std::make_unique<ToolFix>(this, tool_id);

    case ToolID::NOPOPULATE:
    case ToolID::POPULATE:
        return std::make_unique<ToolNoPopulate>(this, tool_id);

    case ToolID::POLYGON_TO_LINE_LOOP:
        return std::make_unique<ToolPolygonToLineLoop>(this, tool_id);

    case ToolID::PLACE_BOARD_PANEL:
        return std::make_unique<ToolPlaceBoardPanel>(this, tool_id);

    case ToolID::SMASH_PANEL_OUTLINE:
        return std::make_unique<ToolSmashPanelOutline>(this, tool_id);

    case ToolID::SMASH_PACKAGE_OUTLINE:
        return std::make_unique<ToolSmashPackageOutline>(this, tool_id);

    case ToolID::RESIZE_SYMBOL:
        return std::make_unique<ToolResizeSymbol>(this, tool_id);

    case ToolID::ROUND_OFF_VERTEX:
        return std::make_unique<ToolRoundOffVertex>(this, tool_id);

    case ToolID::SWAP_GATES:
        return std::make_unique<ToolSwapGates>(this, tool_id);

    case ToolID::PLACE_PICTURE:
        return std::make_unique<ToolPlacePicture>(this, tool_id);

    default:
        return nullptr;
    }
}

} // namespace horizon
