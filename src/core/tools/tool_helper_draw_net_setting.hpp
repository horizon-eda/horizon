#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolHelperDrawNetSetting : public virtual ToolBase {
public:
    ToolHelperDrawNetSetting(IDocument *c, ToolID tid);
    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t net_label_size = 1.5_mm;
    };

    const ToolSettings *get_settings_const() const override
    {
        return &settings;
    }

    ToolID get_tool_id_for_settings() const override;

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
