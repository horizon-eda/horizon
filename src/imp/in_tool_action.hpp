#pragma once

namespace horizon {

enum class InToolActionID {
    NONE,
    // common
    LMB,
    LMB_RELEASE,
    RMB,
    ROTATE,
    ROTATE_CURSOR,
    MIRROR,
    MIRROR_CURSOR,
    CANCEL,
    COMMIT,
    RESTRICT,
    POSTURE,
    ENTER_WIDTH,
    ENTER_SIZE,
    ENTER_DATUM,
    EDIT,
    MOVE_UP,
    MOVE_DOWN,
    MOVE_LEFT,
    MOVE_RIGHT,
    MOVE_UP_FINE,
    MOVE_DOWN_FINE,
    MOVE_LEFT_FINE,
    MOVE_RIGHT_FINE,
    RECTANGLE_MODE,
    NET_LABEL_SIZE_INC,
    NET_LABEL_SIZE_DEC,
    FLIP_ARC,

    // draw polygon
    TOGGLE_ARC,

    // draw dimension
    DIMENSION_MODE,

    // draw line net
    PLACE_JUNCTION,
    ARBITRARY_ANGLE_MODE,
    TOGGLE_NET_LABEL,

    // draw polygon rectangle
    POLYGON_DECORATION_POSITION,
    POLYGON_DECORATION_SIZE,
    POLYGON_DECORATION_STYLE,
    POLYGON_CORNER_RADIUS,

    // place pin
    AUTOPLACE_NEXT_PIN,
    AUTOPLACE_ALL_PINS,

    // rotate arbitrary
    TOGGLE_ANGLE_SNAP,

    // route track interactive
    LENGTH_TUNING_LENGTH,
    LENGTH_TUNING_AMPLITUDE_INC,
    LENGTH_TUNING_AMPLITUDE_DEC,
    LENGTH_TUNING_SPACING_INC,
    LENGTH_TUNING_SPACING_DEC,
    TOGGLE_VIA,
    TRACK_WIDTH_DEFAULT,
    ROUTER_SETTINGS,
    CLEARANCE_OFFSET,
    CLEARANCE_OFFSET_DEFAULT,
    ROUTER_MODE,

    // set NC
    NC_MODE,

    // add vertex
    FLIP_DIRECTION,
};

} // namespace horizon
