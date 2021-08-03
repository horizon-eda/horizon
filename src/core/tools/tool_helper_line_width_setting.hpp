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

    std::map<ToolID, ToolSettings *> get_all_settings() override;

protected:
    void ask_line_width();

    Settings settings;
};
} // namespace horizon
