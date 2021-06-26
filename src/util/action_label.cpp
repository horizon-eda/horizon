#include "action_label.hpp"
#include "imp/in_tool_action.hpp"

namespace horizon {
ActionLabelInfo::ActionLabelInfo(InToolActionID a1) : action1(a1), action2(InToolActionID::NONE)
{
}
ActionLabelInfo::ActionLabelInfo(InToolActionID a1, const std::string &s)
    : action1(a1), action2(InToolActionID::NONE), label(s)
{
}
ActionLabelInfo::ActionLabelInfo(InToolActionID a1, InToolActionID a2, const std::string &s)
    : action1(a1), action2(a2), label(s)
{
}
} // namespace horizon
