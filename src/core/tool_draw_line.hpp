#pragma once
#include "core.hpp"

namespace horizon {

class ToolDrawLine : public ToolBase {
public:
    ToolDrawLine(Core *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool handles_esc() override
    {
        return true;
    }

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

    void apply_settings() override;

protected:
    ToolSettings *get_settings() override
    {
        return &settings;
    }

private:
    Junction *temp_junc = 0;
    Line *temp_line = 0;
    void update_tip();
    bool first_line = true;
    std::set<const Junction *> junctions_created;

    Settings settings;
};
} // namespace horizon
