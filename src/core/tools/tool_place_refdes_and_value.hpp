#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolPlaceRefdesAndValue : public ToolBase {
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
    class Text *text_refdes = nullptr;
    Text *text_value = nullptr;

    void update_texts(const Coordi &c);
};
} // namespace horizon
