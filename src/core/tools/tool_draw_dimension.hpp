#pragma once
#include "core/tool.hpp"
#include "tool_helper_restrict.hpp"

namespace horizon {
class ToolDrawDimension : public ToolBase, public ToolHelperRestrict {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::RESTRICT, I::DIMENSION_MODE, I::ENTER_DATUM, I::ENTER_SIZE,
        };
    }

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t label_size = 1.5_mm;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    void apply_settings() override;

private:
    class Dimension *temp = nullptr;
    void update_tip();

    enum class State { P0, P1, LABEL };
    State state = State::P0;
    Settings settings;
};
} // namespace horizon
