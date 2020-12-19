#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolPlacePad : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::EDIT,
        };
    }

private:
    const class Padstack *padstack = nullptr;
    class Pad *temp = nullptr;
    void create_pad(const Coordi &c);
};
} // namespace horizon
