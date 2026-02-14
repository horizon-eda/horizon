#pragma once
#include "tool_helper_move.hpp"

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
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR,
        };
    }

private:
    class Table *temp = nullptr;
    ToolResponse finish();
};

} // namespace horizon
