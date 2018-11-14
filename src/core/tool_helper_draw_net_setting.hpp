#pragma once
#include "core.hpp"

namespace horizon {

class ToolHelperDrawNetSetting : public virtual ToolBase {
public:
    ToolHelperDrawNetSetting(Core *c, ToolID tid);
    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t net_label_size = 1.5;
    };

    const ToolSettings *get_settings_const() const override
    {
        return &settings;
    }

    ToolID get_tool_id_for_settings() const override
    {
        return ToolID::DRAW_LINE;
    }

protected:
    ToolSettings *get_settings() override
    {
        return &settings;
    }
    void step_net_label_size(bool up);
    void ask_net_label_size();

    Settings settings;
};
} // namespace horizon
