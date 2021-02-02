#pragma once
#include "core/tool.hpp"
#include "clipper/clipper.hpp"

namespace horizon {

class ToolGenerateSilkscreen : public ToolBase {
public:
    using ToolBase::ToolBase;
    ToolResponse begin(const ToolArgs &args) override;
    ToolResponse update(const ToolArgs &args) override;
    bool can_begin() override;
    bool is_specific() override
    {
        return false;
    }

    class Settings : public ToolSettings {
    public:
        json serialize() const override;
        void load_from_json(const json &j) override;
        void load_defaults();
        int64_t expand_silk = .2_mm;
        int64_t expand_pad = .2_mm;
        int64_t line_width = .15_mm;
    };

    ToolSettings *get_settings() override
    {
        return &settings;
    }

    std::set<InToolActionID> get_actions() const override
    {
        using I = InToolActionID;
        return {
                I::LMB,
                I::CANCEL,
        };
    }

private:
    bool select_polygon();
    ToolResponse redraw_silkscreen();
    void clear_silkscreen();
    void restore_package_visibility();

    class GenerateSilkscreenWindow *win = nullptr;
    Settings settings;
    const class Polygon *pp;
    bool package_visible;
    ClipperLib::Path path_pkg;
    ClipperLib::Paths pads;
};
} // namespace horizon
