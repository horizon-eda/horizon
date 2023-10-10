#include "tool_swap_placement.hpp"
#include "document/idocument_board.hpp"
#include "board/board.hpp"
#include <util/selection_util.hpp>
#include <iostream>

namespace horizon {

bool ToolSwapPlacement::can_begin()
{
    if (!doc.b) {
        return false;
    }

    auto packages = get_packages();
    return packages.first && packages.second;
}

std::pair<BoardPackage *, BoardPackage *> ToolSwapPlacement::get_packages()
{
    if (sel_count_type(selection, ObjectType::BOARD_PACKAGE) != 2)
        return {nullptr, nullptr};
    std::vector<BoardPackage *> packages;
    for (const auto &it : selection) {
        if (it.type == ObjectType::BOARD_PACKAGE)
            packages.push_back(&doc.b->get_board()->packages.at(it.uuid));
    }

    auto &p1 = *packages.at(0);
    auto &p2 = *packages.at(1);

    if (p1.fixed || p2.fixed)
        return {nullptr, nullptr};
    return {&p1, &p2};
}

ToolResponse ToolSwapPlacement::begin(const ToolArgs &args)
{
    std::cout << "tool swap placement\n";
    auto packages = get_packages();
    std::swap(packages.first->placement, packages.second->placement);
    return ToolResponse::commit();
}

ToolResponse ToolSwapPlacement::update(const ToolArgs &args)
{
    return ToolResponse();
}

} // namespace horizon
