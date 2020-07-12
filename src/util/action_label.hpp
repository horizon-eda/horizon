#pragma once
#include <string>
#include "imp/in_tool_action.hpp"

namespace horizon {
class ActionLabelInfo {
public:
    ActionLabelInfo(InToolActionID a1) : action1(a1), action2(InToolActionID::NONE)
    {
    }
    ActionLabelInfo(InToolActionID a1, const std::string &s) : action1(a1), action2(InToolActionID::NONE), label(s)
    {
    }
    ActionLabelInfo(InToolActionID a1, InToolActionID a2, const std::string &s) : action1(a1), action2(a2), label(s)
    {
    }

    InToolActionID action1;
    InToolActionID action2;
    std::string label;
    std::pair<InToolActionID, InToolActionID> get_key() const
    {
        return std::make_pair(action1, action2);
    }

    bool operator==(const ActionLabelInfo &other) const
    {
        return action1 == other.action1 && action2 == other.action2 && label == other.label;
    }
};
} // namespace horizon
