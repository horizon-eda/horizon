#pragma once
#include "tool_data.hpp"

namespace horizon {
class ToolDataWindow : public ToolData {
public:
    enum class Event { NONE, CLOSE, OK, UPDATE };
    Event event = Event::NONE;
};
} // namespace horizon
