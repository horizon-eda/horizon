#pragma once
#include "core/tool.hpp"

namespace horizon {
class ToolMapPin : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    ~ToolMapPin();
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR, I::EDIT, I::AUTOPLACE_ALL_PINS, I::AUTOPLACE_NEXT_PIN,
        };
    }

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        Orientation orientation = Orientation::RIGHT;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

private:
    Settings settings;

    std::vector<std::pair<const class Pin *, bool>> pins;
    std::optional<UUID> map_pin_dialog();
    unsigned int pin_index = 0;
    class SymbolPin *pin = nullptr;
    SymbolPin *pin_last = nullptr;
    SymbolPin *pin_last2 = nullptr;
    void create_pin(const UUID &uu);
    bool can_autoplace() const;
    void update_tip();
    class CanvasAnnotation *annotation = nullptr;
    void update_annotation();
};
} // namespace horizon
