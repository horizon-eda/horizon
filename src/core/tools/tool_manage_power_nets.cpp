#include "tool_manage_power_nets.hpp"
#include "document/idocument_schematic.hpp"
#include "imp/imp_interface.hpp"
#include "tool_data_from_place_power_symbol.hpp"
#include "core/tool_id.hpp"
#include "blocks/blocks_schematic.hpp"

namespace horizon {

bool ToolManagePowerNets::can_begin()
{
    return doc.c;
}

ToolResponse ToolManagePowerNets::begin(const ToolArgs &args)
{

    const auto r = imp->dialogs.manage_power_nets(doc.c->get_blocks());

    if (r) {
        if (dynamic_cast<const ToolDataFromPlacePowerSymbol *>(args.data.get()))
            return ToolResponse::next(ToolResponse::Result::COMMIT, ToolID::PLACE_POWER_SYMBOL);
        else
            return ToolResponse::commit();
    }
    else {
        return ToolResponse::revert();
    }
}

ToolResponse ToolManagePowerNets::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
