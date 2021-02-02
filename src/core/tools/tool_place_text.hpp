#pragma once
#include "core/tool.hpp"
#include "tool_helper_move.hpp"
#include <forward_list>
#include <map>

namespace horizon {

class ToolPlaceText : public ToolHelperMove {
public:
    using ToolHelperMove::ToolHelperMove;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override;

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        class LayerSettings {
        public:
            LayerSettings()
            {
            }
            LayerSettings(const json &j);
            uint64_t width = 0;
            uint64_t size = 1.5_mm;
            json serialize() const;
        };
        const LayerSettings &get_layer(int l) const;
        std::map<int, LayerSettings> layers;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    void apply_settings() override;

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB, I::CANCEL, I::RMB, I::EDIT, I::ROTATE, I::MIRROR, I::ENTER_SIZE, I::ENTER_WIDTH,
        };
    }

private:
    class Text *temp = 0;
    class BoardPackage *pkg = nullptr;
    std::forward_list<Text *> texts_placed;
    Settings settings;
};
} // namespace horizon
