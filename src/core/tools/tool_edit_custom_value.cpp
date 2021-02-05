#include "tool_edit_custom_value.hpp"
#include "imp/imp_interface.hpp"

namespace horizon {

bool ToolEditCustomValue::can_begin()
{
    return get_symbol();
}

ToolResponse ToolEditCustomValue::begin(const ToolArgs &args)
{
    auto sym = get_symbol();

    bool r = imp->dialogs.edit_custom_value(*sym);
    if (r) {
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}
ToolResponse ToolEditCustomValue::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
