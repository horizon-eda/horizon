#include "action_catalog.hpp"
#include "core/core.hpp"

namespace horizon {
const std::map<std::pair<ActionID, ToolID>, ActionCatalogItem> action_catalog = {
        {{ActionID::SELECTION_FILTER, ToolID::NONE},
         {"Selection filter", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::PREFERENCES, ToolID::NONE},
         {"Preferences", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::HELP, ToolID::NONE},
         {"Help", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::VIEW_ALL, ToolID::NONE},
         {"View all", ActionGroup::VIEW, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::FLIP_VIEW, ToolID::NONE},
         {"Flip view", ActionGroup::VIEW, ActionCatalogItem::AVAILABLE_IN_PACKAGE_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::VIEW_TOP, ToolID::NONE},
         {"View top", ActionGroup::VIEW, ActionCatalogItem::AVAILABLE_IN_PACKAGE_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::VIEW_BOTTOM, ToolID::NONE},
         {"View bottom", ActionGroup::VIEW, ActionCatalogItem::AVAILABLE_IN_PACKAGE_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::LAYER_DOWN, ToolID::NONE},
         {"Layer down", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_UP, ToolID::NONE},
         {"Layer up", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_TOP, ToolID::NONE},
         {"Layer top", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_BOTTOM, ToolID::NONE},
         {"Layer bottom", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER1, ToolID::NONE},
         {"Layer inner 1", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER2, ToolID::NONE},
         {"Layer inner 2", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER3, ToolID::NONE},
         {"Layer inner 3", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER4, ToolID::NONE},
         {"Layer inner 4", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER5, ToolID::NONE},
         {"Layer inner 5", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER6, ToolID::NONE},
         {"Layer inner 6", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER7, ToolID::NONE},
         {"Layer inner 7", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::LAYER_INNER8, ToolID::NONE},
         {"Layer inner 8", ActionGroup::LAYER, ActionCatalogItem::AVAILABLE_LAYERED, ActionCatalogItem::FLAGS_IN_TOOL}},

        {{ActionID::POPOVER, ToolID::NONE},
         {"Popover", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_NO_POPOVER | ActionCatalogItem::FLAGS_NO_MENU}},

        {{ActionID::TOOL, ToolID::PASTE},
         {"Paste", ActionGroup::CLIPBOARD, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::COPY, ToolID::NONE},
         {"Copy", ActionGroup::CLIPBOARD, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::DUPLICATE, ToolID::NONE},
         {"Duplicate", ActionGroup::CLIPBOARD, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::SAVE, ToolID::NONE},
         {"Save", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::RELOAD_NETLIST, ToolID::NONE},
         {"Reload netlist", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SAVE_RELOAD_NETLIST, ToolID::NONE},
         {"Save and reload netlist", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::UNDO, ToolID::NONE},
         {"Undo", ActionGroup::UNDO, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::REDO, ToolID::NONE},
         {"Redo", ActionGroup::UNDO, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::VIEW_3D, ToolID::NONE},
         {"3D View", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_PACKAGE_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_TOOL_BOX, ToolID::NONE},
         {"Selection tool box", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_TOOL_LASSO, ToolID::NONE},
         {"Selection tool lasso", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_TOOL_PAINT, ToolID::NONE},
         {"Selection tool paint", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_QUALIFIER_AUTO, ToolID::NONE},
         {"Selection qualifier auto", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_QUALIFIER_INCLUDE_BOX, ToolID::NONE},
         {"Selection qualifier include box", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_QUALIFIER_INCLUDE_ORIGIN, ToolID::NONE},
         {"Selection qualifier include origin", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SELECTION_QUALIFIER_TOUCH_BOX, ToolID::NONE},
         {"Selection qualifier touch box", ActionGroup::SELECTION, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TO_BOARD, ToolID::NONE},
         {"Place on board", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::RULES, ToolID::NONE},
         {"Rules", ActionGroup::RULES,
          ActionCatalogItem::AVAILABLE_IN_SCHEMATIC_AND_BOARD | ActionCatalogItem::AVAILABLE_IN_PACKAGE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::RULES_APPLY, ToolID::NONE},
         {"Apply rules", ActionGroup::RULES, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::RULES_RUN_CHECKS, ToolID::NONE},
         {"Run checks", ActionGroup::RULES,
          ActionCatalogItem::AVAILABLE_IN_SCHEMATIC_AND_BOARD | ActionCatalogItem::AVAILABLE_IN_PACKAGE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::SHOW_IN_BROWSER, ToolID::NONE},
         {"Show in browser", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::TUNING, ToolID::NONE},
         {"Length tuning", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TUNING_ADD_TRACKS, ToolID::NONE},
         {"Measure track", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::TUNING_ADD_TRACKS_ALL, ToolID::NONE},
         {"Measure all tracks", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::HIGHLIGHT_NET, ToolID::NONE},
         {"Highlight net", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC_AND_BOARD,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::TOOL, ToolID::MOVE},
         {"Move", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MOVE_EXACTLY},
         {"Move exactly", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ROTATE},
         {"Rotate", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MIRROR_X},
         {"Mirror X", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MIRROR_Y},
         {"Mirror Y", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ROTATE_ARBITRARY},
         {"Rotate arbitrary", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SCALE},
         {"Scale", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ENTER_DATUM},
         {"Enter datum", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DELETE},
         {"Delete", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ADD_COMPONENT},
         {"Place component", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::PLACE_PART, ToolID::NONE},
         {"Place part", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ADD_PART},
         {"Place part", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::BEND_LINE_NET},
         {"Bend net line", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DISCONNECT},
         {"Disconnect", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_ARC},
         {"Draw arc", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_LINE},
         {"Draw line", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_NET},
         {"Draw net line", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_SYMBOL_PIN_NAMES},
         {"Edit symbol pin names", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MANAGE_BUSES},
         {"Manage buses", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MANAGE_BUSES},
         {"Manage buses", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MANAGE_NET_CLASSES},
         {"Manage net classes", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MANAGE_POWER_NETS},
         {"Manage power nets", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MAP_PIN},
         {"Place pin", ActionGroup::SYMBOL, ActionCatalogItem::AVAILABLE_IN_SYMBOL, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SELECT_NET_SEGMENT},
         {"Select net segment", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MOVE_NET_SEGMENT},
         {"Move net segment to other net", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MOVE_NET_SEGMENT_NEW},
         {"Move net segment to new net", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_BUS_LABEL},
         {"Place bus label", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_BUS_RIPPER},
         {"Place bus ripper", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_NET_LABEL},
         {"Place net label", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ANNOTATE},
         {"Annotate", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MAP_SYMBOL},
         {"Place symbol", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::MOVE_TO_OTHER_SHEET, ToolID::NONE},
         {"Move to other sheet", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::TOOL, ToolID::SET_GROUP},
         {"Set group", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_GROUP},
         {"Clear group", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SET_TAG},
         {"Set tag", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_TAG},
         {"Clear tag", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::COPY_PLACEMENT},
         {"Copy placement", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::COPY_TRACKS},
         {"Copy tracks", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SMASH},
         {"Smash", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::UNSMASH},
         {"Unmash", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC_AND_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_HOLE},
         {"Place hole", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_IN_PADSTACK,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_SHAPE},
         {"Place shape", ActionGroup::PADSTACK, ActionCatalogItem::AVAILABLE_IN_PADSTACK,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_SHAPE},
         {"Edit shape", ActionGroup::PADSTACK, ActionCatalogItem::AVAILABLE_IN_PADSTACK,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::IMPORT_DXF},
         {"Import dxf", ActionGroup::UNKNOWN,
          ActionCatalogItem::AVAILABLE_IN_BOARD | ActionCatalogItem::AVAILABLE_IN_PACKAGE
                  | ActionCatalogItem::AVAILABLE_IN_FRAME,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_JUNCTION},
         {"Place junction", ActionGroup::UNKNOWN, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_POLYGON},
         {"Draw polygon", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_POLYGON_RECTANGLE},
         {"Draw polygon rectangle", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_LINE_RECTANGLE},
         {"Draw line rectangle", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_LINE_RECTANGLE},
         {"Edit line rectangle", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_PAD},
         {"Place pad", ActionGroup::PACKAGE, ActionCatalogItem::AVAILABLE_IN_PACKAGE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_PAD_PARAMETER_SET},
         {"Edit pad", ActionGroup::PACKAGE, ActionCatalogItem::AVAILABLE_IN_PACKAGE, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_POWER_SYMBOL},
         {"Place power symbol", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_TEXT},
         {"Place text", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ASSIGN_PART},
         {"Assign part", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_SCHEMATIC_PROPERTIES},
         {"Edit schematic properties", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SET_DIFFPAIR},
         {"Set diff. pair", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_DIFFPAIR},
         {"Clear diff. pair", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MAP_PACKAGE},
         {"Place package", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_VIA},
         {"Place via", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_VIA},
         {"Edit via", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ROUTE_TRACK},
         {"Route track (old)", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_TRACK},
         {"Draw track", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ROUTE_TRACK_INTERACTIVE},
         {"Route track", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ROUTE_DIFFPAIR_INTERACTIVE},
         {"Route diff. pair", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAG_KEEP_SLOPE},
         {"Drag and keep slope", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAG_TRACK_INTERACTIVE},
         {"Drag track", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::TUNE_TRACK},
         {"Tune track", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::TUNE_DIFFPAIR},
         {"Tune diff. pair", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::TUNE_DIFFPAIR_SKEW},
         {"Tune diff. pair skew", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ADD_PLANE},
         {"Add plane", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_PLANE},
         {"Edit plane", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_PLANE},
         {"Clear plane", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_ALL_PLANES},
         {"Clear all planes", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::UPDATE_PLANE},
         {"Update plane", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::UPDATE_ALL_PLANES},
         {"Update all planes", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_STACKUP},
         {"Edit stackup", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::DRAW_DIMENSION},
         {"Draw dimension", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SELECT_MORE},
         {"Select more", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SELECT_MORE_NO_VIA},
         {"Select more (stop at vias)", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SET_VIA_NET},
         {"Set via net", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_VIA_NET},
         {"Clear via net", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CLEAR_VIA_NET},
         {"Clear via net", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::LOCK},
         {"Lock", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::UNLOCK},
         {"Unlock", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::UNLOCK_ALL},
         {"Unlock all", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::ADD_VERTEX},
         {"Add vertex", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::PLACE_BOARD_HOLE},
         {"Place hole", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_BOARD_HOLE},
         {"Edit hole", ActionGroup::BOARD, ActionCatalogItem::AVAILABLE_IN_BOARD, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::GENERATE_COURTYARD},
         {"Generate courtyard", ActionGroup::PACKAGE, ActionCatalogItem::AVAILABLE_IN_PACKAGE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MOVE_KEY},
         {"Move by keyboard", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::MOVE_KEY_UP},
         {"Move up", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_NO_MENU | ActionCatalogItem::FLAGS_NO_POPOVER}},

        {{ActionID::TOOL, ToolID::MOVE_KEY_DOWN},
         {"Move down", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_NO_MENU | ActionCatalogItem::FLAGS_NO_POPOVER}},

        {{ActionID::TOOL, ToolID::MOVE_KEY_LEFT},
         {"Move left", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_NO_MENU | ActionCatalogItem::FLAGS_NO_POPOVER}},

        {{ActionID::TOOL, ToolID::MOVE_KEY_RIGHT},
         {"Move right", ActionGroup::MOVE, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_NO_MENU | ActionCatalogItem::FLAGS_NO_POPOVER}},

        {{ActionID::BOM_EXPORT_WINDOW, ToolID::NONE},
         {"BOM export window", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::EXPORT_BOM, ToolID::NONE},
         {"Export BOM", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::SWAP_NETS},
         {"Swap nets", ActionGroup::SCHEMATIC, ActionCatalogItem::AVAILABLE_IN_SCHEMATIC,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::LINE_LOOP_TO_POLYGON},
         {"Line loop to polygon", ActionGroup::GRAPHICS, ActionCatalogItem::AVAILABLE_EVERYWHERE,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::RELOAD_POOL, ToolID::NONE},
         {"Reload pool", ActionGroup::UNKNOWN,
          ActionCatalogItem::AVAILABLE_IN_PACKAGE | ActionCatalogItem::AVAILABLE_IN_SYMBOL,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::EDIT_PADSTACK, ToolID::NONE},
         {"Edit padstack", ActionGroup::PACKAGE, ActionCatalogItem::AVAILABLE_IN_PACKAGE,
          ActionCatalogItem::FLAGS_SPECIFIC}},

        {{ActionID::EDIT_UNIT, ToolID::NONE},
         {"Edit unit", ActionGroup::SYMBOL, ActionCatalogItem::AVAILABLE_IN_SYMBOL, ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::CHANGE_UNIT},
         {"Change unit", ActionGroup::SYMBOL, ActionCatalogItem::AVAILABLE_IN_SYMBOL,
          ActionCatalogItem::FLAGS_DEFAULT}},

        {{ActionID::TOOL, ToolID::EDIT_FRAME_PROPERTIES},
         {"Edit frame properties", ActionGroup::FRAME, ActionCatalogItem::AVAILABLE_IN_FRAME,
          ActionCatalogItem::FLAGS_DEFAULT}},
};

const std::vector<std::pair<ActionGroup, std::string>> action_group_catalog = {
        {ActionGroup::CLIPBOARD, "Clipboard"}, {ActionGroup::GRAPHICS, "Graphics"},
        {ActionGroup::MOVE, "Move"},           {ActionGroup::BOARD, "Board"},
        {ActionGroup::SCHEMATIC, "Schematic"}, {ActionGroup::SYMBOL, "Symbol"},
        {ActionGroup::PADSTACK, "Padstack"},   {ActionGroup::PACKAGE, "Package"},
        {ActionGroup::FRAME, "Fame"},          {ActionGroup::UNDO, "Undo"},
        {ActionGroup::LAYER, "Layer"},         {ActionGroup::SELECTION, "Selection"},
        {ActionGroup::RULES, "Rules"},         {ActionGroup::UNKNOWN, "Misc"},
        {ActionGroup::VIEW, "View"},

};

#define ACTION_LUT_ITEM(x)                                                                                             \
    {                                                                                                                  \
        #x, ActionID::x                                                                                                \
    }

const LutEnumStr<ActionID> action_lut = {
        ACTION_LUT_ITEM(TOOL),
        ACTION_LUT_ITEM(SELECTION_FILTER),
        ACTION_LUT_ITEM(SAVE),
        ACTION_LUT_ITEM(VIEW_3D),
        ACTION_LUT_ITEM(UNDO),
        ACTION_LUT_ITEM(REDO),
        ACTION_LUT_ITEM(COPY),
        ACTION_LUT_ITEM(DUPLICATE),
        ACTION_LUT_ITEM(PREFERENCES),
        ACTION_LUT_ITEM(PLACE_PART),
        ACTION_LUT_ITEM(HELP),
        ACTION_LUT_ITEM(RULES),
        ACTION_LUT_ITEM(RULES_APPLY),
        ACTION_LUT_ITEM(RULES_RUN_CHECKS),
        ACTION_LUT_ITEM(LAYER_UP),
        ACTION_LUT_ITEM(LAYER_DOWN),
        ACTION_LUT_ITEM(LAYER_INNER1),
        ACTION_LUT_ITEM(LAYER_INNER2),
        ACTION_LUT_ITEM(LAYER_INNER3),
        ACTION_LUT_ITEM(LAYER_INNER4),
        ACTION_LUT_ITEM(LAYER_INNER5),
        ACTION_LUT_ITEM(LAYER_INNER6),
        ACTION_LUT_ITEM(LAYER_INNER7),
        ACTION_LUT_ITEM(LAYER_INNER8),
        ACTION_LUT_ITEM(LAYER_TOP),
        ACTION_LUT_ITEM(LAYER_BOTTOM),
        ACTION_LUT_ITEM(POPOVER),
        ACTION_LUT_ITEM(VIEW_ALL),
        ACTION_LUT_ITEM(SELECTION_TOOL_BOX),
        ACTION_LUT_ITEM(SELECTION_TOOL_LASSO),
        ACTION_LUT_ITEM(SELECTION_TOOL_PAINT),
        ACTION_LUT_ITEM(SELECTION_QUALIFIER_AUTO),
        ACTION_LUT_ITEM(SELECTION_QUALIFIER_INCLUDE_ORIGIN),
        ACTION_LUT_ITEM(SELECTION_QUALIFIER_TOUCH_BOX),
        ACTION_LUT_ITEM(SELECTION_QUALIFIER_INCLUDE_BOX),
        ACTION_LUT_ITEM(TO_BOARD),
        ACTION_LUT_ITEM(MOVE_TO_OTHER_SHEET),
        ACTION_LUT_ITEM(SHOW_IN_BROWSER),
        ACTION_LUT_ITEM(TUNING),
        ACTION_LUT_ITEM(TUNING_ADD_TRACKS),
        ACTION_LUT_ITEM(TUNING_ADD_TRACKS_ALL),
        ACTION_LUT_ITEM(HIGHLIGHT_NET),
        ACTION_LUT_ITEM(RELOAD_NETLIST),
        ACTION_LUT_ITEM(SAVE_RELOAD_NETLIST),
        ACTION_LUT_ITEM(BOM_EXPORT_WINDOW),
        ACTION_LUT_ITEM(EXPORT_BOM),
        ACTION_LUT_ITEM(RELOAD_POOL),
        ACTION_LUT_ITEM(FLIP_VIEW),
        ACTION_LUT_ITEM(VIEW_TOP),
        ACTION_LUT_ITEM(VIEW_BOTTOM),
        ACTION_LUT_ITEM(EDIT_PADSTACK),
        ACTION_LUT_ITEM(EDIT_UNIT),
};

#define TOOL_LUT_ITEM(x)                                                                                               \
    {                                                                                                                  \
        #x, ToolID::x                                                                                                  \
    }

const LutEnumStr<ToolID> tool_lut = {
        TOOL_LUT_ITEM(EDIT_SCHEMATIC_PROPERTIES),
        TOOL_LUT_ITEM(NONE),
        TOOL_LUT_ITEM(MOVE),
        TOOL_LUT_ITEM(PLACE_JUNCTION),
        TOOL_LUT_ITEM(DRAW_LINE),
        TOOL_LUT_ITEM(DELETE),
        TOOL_LUT_ITEM(DRAW_ARC),
        TOOL_LUT_ITEM(ROTATE),
        {"MIRROR", ToolID::MIRROR_X},
        TOOL_LUT_ITEM(MIRROR_Y),
        TOOL_LUT_ITEM(MAP_PIN),
        TOOL_LUT_ITEM(MAP_SYMBOL),
        TOOL_LUT_ITEM(DRAW_NET),
        TOOL_LUT_ITEM(ADD_COMPONENT),
        TOOL_LUT_ITEM(PLACE_TEXT),
        TOOL_LUT_ITEM(PLACE_NET_LABEL),
        TOOL_LUT_ITEM(DISCONNECT),
        TOOL_LUT_ITEM(BEND_LINE_NET),
        TOOL_LUT_ITEM(SELECT_NET_SEGMENT),
        TOOL_LUT_ITEM(SELECT_NET),
        TOOL_LUT_ITEM(PLACE_POWER_SYMBOL),
        TOOL_LUT_ITEM(MOVE_NET_SEGMENT),
        TOOL_LUT_ITEM(MOVE_NET_SEGMENT_NEW),
        TOOL_LUT_ITEM(EDIT_SYMBOL_PIN_NAMES),
        TOOL_LUT_ITEM(PLACE_BUS_LABEL),
        TOOL_LUT_ITEM(PLACE_BUS_RIPPER),
        TOOL_LUT_ITEM(MANAGE_BUSES),
        TOOL_LUT_ITEM(DRAW_POLYGON),
        TOOL_LUT_ITEM(ENTER_DATUM),
        TOOL_LUT_ITEM(MOVE_EXACTLY),
        TOOL_LUT_ITEM(PLACE_HOLE),
        TOOL_LUT_ITEM(PLACE_PAD),
        TOOL_LUT_ITEM(PASTE),
        TOOL_LUT_ITEM(ASSIGN_PART),
        TOOL_LUT_ITEM(MAP_PACKAGE),
        TOOL_LUT_ITEM(DRAW_TRACK),
        TOOL_LUT_ITEM(PLACE_VIA),
        TOOL_LUT_ITEM(ROUTE_TRACK),
        TOOL_LUT_ITEM(DRAG_KEEP_SLOPE),
        TOOL_LUT_ITEM(ADD_PART),
        TOOL_LUT_ITEM(ANNOTATE),
        TOOL_LUT_ITEM(SMASH),
        TOOL_LUT_ITEM(UNSMASH),
        TOOL_LUT_ITEM(PLACE_SHAPE),
        TOOL_LUT_ITEM(EDIT_SHAPE),
        TOOL_LUT_ITEM(IMPORT_DXF),
        TOOL_LUT_ITEM(MANAGE_NET_CLASSES),
        TOOL_LUT_ITEM(EDIT_PARAMETER_SET),
        TOOL_LUT_ITEM(EDIT_PARAMETER_PROGRAM),
        TOOL_LUT_ITEM(EDIT_PAD_PARAMETER_SET),
        TOOL_LUT_ITEM(DRAW_POLYGON_RECTANGLE),
        TOOL_LUT_ITEM(DRAW_LINE_RECTANGLE),
        TOOL_LUT_ITEM(EDIT_LINE_RECTANGLE),
        TOOL_LUT_ITEM(ROUTE_TRACK_INTERACTIVE),
        TOOL_LUT_ITEM(EDIT_VIA),
        TOOL_LUT_ITEM(ROTATE_ARBITRARY),
        TOOL_LUT_ITEM(ADD_PLANE),
        TOOL_LUT_ITEM(EDIT_PLANE),
        TOOL_LUT_ITEM(UPDATE_PLANE),
        TOOL_LUT_ITEM(UPDATE_ALL_PLANES),
        TOOL_LUT_ITEM(CLEAR_PLANE),
        TOOL_LUT_ITEM(CLEAR_ALL_PLANES),
        TOOL_LUT_ITEM(EDIT_STACKUP),
        TOOL_LUT_ITEM(DRAW_DIMENSION),
        TOOL_LUT_ITEM(SET_DIFFPAIR),
        TOOL_LUT_ITEM(CLEAR_DIFFPAIR),
        TOOL_LUT_ITEM(ROUTE_DIFFPAIR_INTERACTIVE),
        TOOL_LUT_ITEM(SELECT_MORE),
        TOOL_LUT_ITEM(SELECT_MORE_NO_VIA),
        TOOL_LUT_ITEM(SET_VIA_NET),
        TOOL_LUT_ITEM(CLEAR_VIA_NET),
        TOOL_LUT_ITEM(DRAG_TRACK_INTERACTIVE),
        TOOL_LUT_ITEM(LOCK),
        TOOL_LUT_ITEM(UNLOCK),
        TOOL_LUT_ITEM(UNLOCK_ALL),
        TOOL_LUT_ITEM(ADD_VERTEX),
        TOOL_LUT_ITEM(MANAGE_POWER_NETS),
        TOOL_LUT_ITEM(PLACE_BOARD_HOLE),
        TOOL_LUT_ITEM(EDIT_BOARD_HOLE),
        TOOL_LUT_ITEM(GENERATE_COURTYARD),
        TOOL_LUT_ITEM(SET_GROUP),
        TOOL_LUT_ITEM(CLEAR_GROUP),
        TOOL_LUT_ITEM(SET_TAG),
        TOOL_LUT_ITEM(CLEAR_TAG),
        TOOL_LUT_ITEM(APPLY_TAG),
        TOOL_LUT_ITEM(SELECT_GROUP),
        TOOL_LUT_ITEM(HIGHLIGHT_GROUP),
        TOOL_LUT_ITEM(SELECT_TAG),
        TOOL_LUT_ITEM(HIGHLIGHT_TAG),
        TOOL_LUT_ITEM(COPY_PLACEMENT),
        TOOL_LUT_ITEM(COPY_TRACKS),
        TOOL_LUT_ITEM(TUNE_TRACK),
        TOOL_LUT_ITEM(TUNE_DIFFPAIR),
        TOOL_LUT_ITEM(TUNE_DIFFPAIR_SKEW),
        TOOL_LUT_ITEM(MOVE_KEY),
        TOOL_LUT_ITEM(MOVE_KEY_UP),
        TOOL_LUT_ITEM(MOVE_KEY_DOWN),
        TOOL_LUT_ITEM(MOVE_KEY_LEFT),
        TOOL_LUT_ITEM(MOVE_KEY_RIGHT),
        TOOL_LUT_ITEM(SWAP_NETS),
        TOOL_LUT_ITEM(LINE_LOOP_TO_POLYGON),
        TOOL_LUT_ITEM(SCALE),
        TOOL_LUT_ITEM(CHANGE_UNIT),
        TOOL_LUT_ITEM(EDIT_FRAME_PROPERTIES),
};
} // namespace horizon
