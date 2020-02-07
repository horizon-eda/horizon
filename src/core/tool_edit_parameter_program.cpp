#include "tool_edit_parameter_program.hpp"
#include "idocument_padstack.hpp"
#include "pool/padstack.hpp"
#include <iostream>

namespace horizon {

ToolEditParameterProgram::ToolEditParameterProgram(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolEditParameterProgram::can_begin()
{
    return doc.a;
}

ToolResponse ToolEditParameterProgram::begin(const ToolArgs &args)
{
    bool r;
    auto ps = doc.a->get_padstack();
    if (tool_id == ToolID::EDIT_PARAMETER_PROGRAM) {
        r = imp->dialogs.edit_parameter_program(&ps->parameter_program);
    }
    else {
        r = imp->dialogs.edit_parameter_set(&ps->parameter_set);
    }
    if (r) {
        return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}
ToolResponse ToolEditParameterProgram::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
