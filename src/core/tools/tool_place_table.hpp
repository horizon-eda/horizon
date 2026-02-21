#pragma once
#include "tool_helper_move.hpp"

#include <forward_list>
#include <iostream>
#include <ostream>

namespace horizon {

class ToolPlaceTable : public ToolHelperMove {
public:
    using ToolHelperMove::ToolHelperMove;

    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::EDIT, I::ROTATE,
        };
    }

private:
    class Table *temp = nullptr;
    std::forward_list<Table *> tables_placed;
    ToolResponse finish();
};

} // namespace horizon
