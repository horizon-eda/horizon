#include "tool_edit_pad_parameter_set.hpp"
#include "core_package.hpp"
#include "imp/imp_interface.hpp"
#include <iostream>

namespace horizon {

ToolEditPadParameterSet::ToolEditPadParameterSet(Core *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolEditPadParameterSet::can_begin()
{
    return get_pads().size() > 0;
}

std::set<Pad *> ToolEditPadParameterSet::get_pads()
{
    std::set<Pad *> pads;
    for (const auto &it : core.r->selection) {
        if (it.type == ObjectType::PAD) {
            pads.emplace(&core.k->get_package()->pads.at(it.uuid));
        }
    }
    return pads;
}

ToolResponse ToolEditPadParameterSet::begin(const ToolArgs &args)
{
    auto pads = get_pads();
    auto r = imp->dialogs.edit_pad_parameter_set(pads, core.r->m_pool, core.k->get_package());
    if (r) {
        core.r->commit();
    }
    else {
        core.r->revert();
    }
    return ToolResponse::end();
}
ToolResponse ToolEditPadParameterSet::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
