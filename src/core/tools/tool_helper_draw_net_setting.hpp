#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolHelperDrawNetSetting : public virtual ToolBase {
public:
    using ToolBase::ToolBase;
    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t net_label_size = 1.5_mm;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    ToolID get_tool_id_for_settings() const override;

protected:
    void step_net_label_size(bool up);
    void ask_net_label_size();

    Settings settings;
};
} // namespace horizon
