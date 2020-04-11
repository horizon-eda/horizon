#include "tool_helper_line_width_setting.hpp"
#include "nlohmann/json.hpp"
#include "imp/imp_interface.hpp"
#include "core/tool_id.hpp"

namespace horizon {

ToolHelperLineWidthSetting::ToolHelperLineWidthSetting(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

ToolID ToolHelperLineWidthSetting::get_tool_id_for_settings() const
{
    return ToolID::DRAW_LINE;
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
    auto r = imp->dialogs.ask_datum("Enter width", settings.width);
    if (r.first) {
        settings.width = std::max(r.second, (int64_t)0);
        apply_settings();
    }
}

} // namespace horizon
