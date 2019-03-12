#pragma once
#include <string>
#include "common/common.hpp"

namespace horizon {
class ToolHelperRestrict {
protected:
    void cycle_restrict_mode();
    void cycle_restrict_mode_xy();
    std::string restrict_mode_to_string() const;
    enum class RestrictMode { X, Y, ARB };
    RestrictMode restrict_mode = RestrictMode::ARB;
    Coordi get_coord_restrict(const Coordi &old, const Coordi &cur) const;
};
} // namespace horizon
