#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolMeasure : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override
    {
        return true;
    }
    bool is_specific() override
    {
        return false;
    }
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
                I::RMB,
        };
    }
    ~ToolMeasure();

private:
    class CanvasAnnotation *annotation = nullptr;

    enum class State { FROM, TO, DONE };
    State state = State::FROM;
    void update_tip();

    Coordi from;
    Coordi to;

    std::string s_from;
    std::string s_to;
};
} // namespace horizon
