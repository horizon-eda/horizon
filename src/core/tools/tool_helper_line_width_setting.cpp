#include "tool_helper_line_width_setting.hpp"
#include <nlohmann/json.hpp>
#include "imp/imp_interface.hpp"
#include "core/tool_id.hpp"

namespace horizon {

std::map<ToolID, ToolSettings *> ToolHelperLineWidthSetting::get_all_settings()
{
    return {{ToolID::DRAW_LINE, &settings}};
}

void ToolHelperLineWidthSetting::Settings::load_from_json(const json &j)
{
    width = j.value("width", 0);
}

json ToolHelperLineWidthSetting::Settings::serialize() const
{
    json j;
    j["width"] = width;
    return j;
}

void ToolHelperLineWidthSetting::ask_line_width()
{
    if (auto r = imp->dialogs.ask_datum("Enter width", settings.width)) {
        settings.width = std::max(*r, (int64_t)0);
        apply_settings();
    }
}

} // namespace horizon
