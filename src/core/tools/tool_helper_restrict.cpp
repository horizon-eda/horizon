#include "tool_helper_restrict.hpp"

namespace horizon {

static Coordi find_45deg_coord(const Coordi &old, const Coordi &cur)
{
    Coordi result;
    const Coordi vect = cur - old;
    const Coordi vect_abs = {std::abs(vect.x), std::abs(vect.y)};

    if (vect_abs.x >= vect_abs.y) {
        result.x = cur.x;
        if (vect_abs.y <= std::abs(vect_abs.x - vect_abs.y)) {
            result.y = old.y;
        }
        else {
            auto a_x = vect.x;

            if ((vect.y < 0 && vect.x > 0) || (vect.y > 0 && vect.x < 0))
                a_x = -a_x;

            result.y = old.y + a_x;
        }
    }
    else {
        result.y = cur.y;
        if (vect_abs.x <= std::abs(vect_abs.y - vect_abs.x)) {
            result.x = old.x;
        }
        else {
            auto a_y = vect.y;

            if ((vect.x < 0 && vect.y > 0) || (vect.x > 0 && vect.y < 0))
                a_y = -a_y;

            result.x = old.x + a_y;
        }
    }

    return result;
}

void ToolHelperRestrict::cycle_restrict_mode()
{
    switch (restrict_mode) {
    case RestrictMode::ARB:
        restrict_mode = RestrictMode::X;
        break;
    case RestrictMode::X:
        restrict_mode = RestrictMode::Y;
        break;
    case RestrictMode::Y:
        restrict_mode = RestrictMode::ARB;
        break;
    case RestrictMode::DEG45:
        restrict_mode = RestrictMode::ARB;
        break;
    }
}

void ToolHelperRestrict::cycle_restrict_mode_xy()
{
    switch (restrict_mode) {
    case RestrictMode::X:
        restrict_mode = RestrictMode::Y;
        break;
    case RestrictMode::Y:
        restrict_mode = RestrictMode::X;
        break;
    default:;
    }
}

void ToolHelperRestrict::toogle_45_degrees_mode()
{
    if (restrict_mode == RestrictMode::DEG45) {
        restrict_mode = RestrictMode::ARB;
    }
    else {
        restrict_mode = RestrictMode::DEG45;
    }
}

std::string ToolHelperRestrict::restrict_mode_to_string() const
{
    switch (restrict_mode) {
    case RestrictMode::ARB:
        return "any direction";
        break;
    case RestrictMode::X:
        return "X only";
        break;
    case RestrictMode::Y:
        return "Y only";
        break;
    case RestrictMode::DEG45:
        return "45° only";
        break;
    }
    return "";
}

Coordi ToolHelperRestrict::get_coord_restrict(const Coordi &old, const Coordi &cur) const
{
    switch (restrict_mode) {
    case RestrictMode::ARB:
        return cur;
    case RestrictMode::X:
        return {cur.x, old.y};
    case RestrictMode::Y:
        return {old.x, cur.y};
    case RestrictMode::DEG45:
        return find_45deg_coord(old, cur);
    }
    return cur;
}

} // namespace horizon
