#pragma once
#include "core/tool.hpp"
#include "tool_helper_move.hpp"

namespace horizon {
class ToolMapPackage : public ToolHelperMove {
public:
    using ToolHelperMove::ToolHelperMove;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::ROTATE, I::MIRROR, I::EDIT,
        };
    }

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        bool flip = false;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

private:
    Settings settings;

    std::vector<std::pair<class Component *, bool>> components;
    unsigned int component_index = 0;
    class BoardPackage *pkg = nullptr;
    std::set<UUID> nets;
    std::set<UUID> all_nets;
    void place_package(Component *comp, const Coordi &c);
    void update_tooltip();
    int angle = 0;
};
} // namespace horizon
