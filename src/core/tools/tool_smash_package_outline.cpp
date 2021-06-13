#include "tool_smash_package_outline.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include "board/board_layers.hpp"
#include <iostream>

namespace horizon {

static bool can_smash(const Package &pkg)
{
    return std::any_of(pkg.polygons.begin(), pkg.polygons.end(),
                       [](const auto &x) { return x.second.layer == BoardLayers::L_OUTLINE; });
}

bool ToolSmashPackageOutline::can_begin()
{
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PACKAGE) {
            const auto &pkg = doc.b->get_board()->packages.at(it.uuid);
            return can_smash(pkg.package) && pkg.omit_outline == false;
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
