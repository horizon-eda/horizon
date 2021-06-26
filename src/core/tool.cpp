#include "tool.hpp"
#include "tool_id.hpp"
#include "imp/in_tool_action.hpp"

namespace horizon {

ToolBase::ToolBase(IDocument *c, ToolID tid) : doc(c), tool_id(tid)
{
}

void ToolBase::set_imp_interface(ImpInterface *i)
{
    if (imp == nullptr) {
        imp = i;
    }
}

void ToolBase::set_transient()
{
    is_transient = true;
}

ToolResponse::ToolResponse() : next_tool(ToolID::NONE)
{
}

ToolResponse::ToolResponse(ToolResponse::Result r) : next_tool(ToolID::NONE), result(r)
{
}

ToolArgs::ToolArgs() : action(InToolActionID::NONE)
{
}

} // namespace horizon
