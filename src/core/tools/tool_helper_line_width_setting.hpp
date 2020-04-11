#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolHelperLineWidthSetting : public ToolBase {
public:
    ToolHelperLineWidthSetting(IDocument *c, ToolID tid);
    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t width = 0;
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
    void ask_line_width();

    Settings settings;
};
} // namespace horizon
