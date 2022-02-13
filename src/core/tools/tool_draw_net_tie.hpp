#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolDrawNetTie : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }

private:
    class BoardJunction *temp_junc = nullptr;
    class BoardNetTie *temp_tie = nullptr;
    class NetTie *net_tie = nullptr;
    void create_temp_tie();
    class BoardRules *rules;
};
} // namespace horizon
