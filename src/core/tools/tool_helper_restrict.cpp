#include "tool_helper_restrict.hpp"

namespace horizon {
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
    }
    return cur;
}

} // namespace horizon
