#include "tool_smash_package_outline.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <iostream>

namespace horizon {

ToolSmashPackageOutline::ToolSmashPackageOutline(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

bool ToolSmashPackageOutline::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            return doc.b->get_board()->packages.at(it.uuid).omit_outline == false;
        }
    }
    return false;
}

ToolResponse ToolSmashPackageOutline::begin(const ToolArgs &args)
{
    for (const auto &it : args.selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            doc.b->get_board()->smash_package_outline(doc.b->get_board()->packages.at(it.uuid));
        }
    }
    return ToolResponse::commit();
}

ToolResponse ToolSmashPackageOutline::update(const ToolArgs &args)
{
    return ToolResponse();
}
} // namespace horizon
