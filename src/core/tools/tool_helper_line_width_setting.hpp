#pragma once
#include "core/tool.hpp"

namespace horizon {

class ToolHelperLineWidthSetting : public ToolBase {
public:
    using ToolBase::ToolBase;
    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t width = 0;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    ToolID get_tool_id_for_settings() const override;

protected:
    void ask_line_width();

    Settings settings;
};
} // namespace horizon
