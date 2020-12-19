#pragma once
#include "board/track.hpp"
#include "core/tool.hpp"

namespace horizon {

class ToolDrawTrack : public ToolBase {
public:
    ToolDrawTrack(IDocument *c, ToolID tid);
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
    class BoardJunction *temp_junc = 0;
    Track *temp_track = 0;
    Track *create_temp_track();
    class BoardRules *rules;
};
} // namespace horizon
