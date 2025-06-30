#include "tool_settings_rectangle_mode.hpp"
#include <nlohmann/json.hpp>

namespace horizon {

static const LutEnumStr<ToolSettingsRectangleMode::Mode> mode_lut = {
        {"center", ToolSettingsRectangleMode::Mode::CENTER},
        {"corner", ToolSettingsRectangleMode::Mode::CORNER},
};

void ToolSettingsRectangleMode::load_from_json(const json &j)
{
    mode = mode_lut.lookup(j.at("mode"));
}

void ToolSettingsRectangleMode::cycle_mode()
{
    mode = mode == Mode::CENTER ? Mode::CORNER : Mode::CENTER;
}

json ToolSettingsRectangleMode::serialize() const
{
    json j;
    j["mode"] = mode_lut.lookup_reverse(mode);
    return j;
}
} // namespace horizon
