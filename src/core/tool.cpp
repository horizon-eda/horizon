#include "tool.hpp"
#include "tool_id.hpp"

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

ToolSettingsProxy::~ToolSettingsProxy()
{
    tool->apply_settings();
}

ToolResponse::ToolResponse() : next_tool(ToolID::NONE)
{
}

ToolResponse::ToolResponse(ToolResponse::Result r) : next_tool(ToolID::NONE), result(r)
{
}

} // namespace horizon
