#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolResizeSymbol : public ToolBase {
public:
    ToolResizeSymbol(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::MOVE_UP, I::MOVE_DOWN, I::MOVE_LEFT, I::MOVE_RIGHT,
        };
    }


private:
    Coordi pos_orig;
    Coordi delta_key;
    std::map<std::pair<ObjectType, UUID>, Coordi> positions;

    void update_positions(const Coordi &ac);
};
} // namespace horizon
