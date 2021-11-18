#pragma once
#include "common/common.hpp"

namespace horizon {
class KeepSlopeInfo {
public:
    struct Position {
        Coordi from;
        Coordi to;
        Coordi arc_center;
    };
    Position get_pos(const Coordd &shift) const;

protected:
    Coordi pos_from2;
    Coordi pos_to2;
    Coordi pos_from_orig;
    Coordi pos_to_orig;
};
} // namespace horizon
