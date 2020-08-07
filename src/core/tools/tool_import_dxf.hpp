#pragma once
#include "core/tool.hpp"
#include "tool_helper_move.hpp"

namespace horizon {

class ToolImportDXF : public ToolHelperMove {
public:
    ToolImportDXF(IDocument *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR, I::ENTER_WIDTH,
        };
    }

private:
    std::set<class Line *> lines;
    std::set<class Arc *> arcs;
    int64_t width = 0;
};
} // namespace horizon
