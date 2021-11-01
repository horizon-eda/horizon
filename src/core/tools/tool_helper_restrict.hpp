#pragma once
#include <string>
#include "common/common.hpp"

namespace horizon {
class ToolHelperRestrict {
protected:
    void cycle_restrict_mode();
    void cycle_restrict_mode_xy();
    void toogle_45_degrees_mode();
    std::string restrict_mode_to_string() const;
    enum class RestrictMode { X, Y, ARB, DEG45 };
    RestrictMode restrict_mode = RestrictMode::ARB;
    Coordi get_coord_restrict(const Coordi &old, const Coordi &cur) const;
    static Coordi find_45deg_coord(const Coordi &old, const Coordi &cur);
};
} // namespace horizon
