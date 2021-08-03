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

    std::map<ToolID, ToolSettings *> get_all_settings() override;

protected:
    void step_net_label_size(bool up);
    void ask_net_label_size();

    Settings settings;
};
} // namespace horizon
