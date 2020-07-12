#pragma once
#include "common/hole.hpp"
#include "core/tool.hpp"
#include <forward_list>

namespace horizon {

class ToolPlaceHole : public ToolBase {
public:
    ToolPlaceHole(IDocument *c, ToolID tid);
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

protected:
    Hole *temp = 0;
    std::forward_list<Hole *> holes_placed;

    void create_hole(const Coordi &c);
};
} // namespace horizon
