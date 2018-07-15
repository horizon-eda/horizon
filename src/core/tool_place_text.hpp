#pragma once
#include "core.hpp"
#include "tool_helper_move.hpp"
#include <forward_list>

namespace horizon {

class ToolPlaceText : public ToolHelperMove {
public:
    ToolPlaceText(Core *c, ToolID tid);
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        uint64_t width = 0;
        uint64_t size = 1.5_mm;
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
    Text *temp = 0;
    std::forward_list<Text *> texts_placed;
    Settings settings;
};
} // namespace horizon
