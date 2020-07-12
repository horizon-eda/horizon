#pragma once
#include "core/tool.hpp"
#include <forward_list>
#include <map>

namespace horizon {

class ToolPlaceRefdesAndValue : public ToolBase {
public:
    ToolPlaceRefdesAndValue(IDocument *c, ToolID tid);
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
    class Text *text_refdes = nullptr;
    Text *text_value = nullptr;

    void update_texts(const Coordi &c);
};
} // namespace horizon
