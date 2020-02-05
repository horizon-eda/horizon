#include "tool.hpp"

namespace horizon {

ToolBase::ToolBase(IDocument *c, ToolID tid) : core(c), tool_id(tid)
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
} // namespace horizon
