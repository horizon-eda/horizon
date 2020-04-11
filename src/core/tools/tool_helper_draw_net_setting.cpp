#include "tool_helper_draw_net_setting.hpp"
#include "nlohmann/json.hpp"
#include "imp/imp_interface.hpp"
#include "core/tool_id.hpp"

namespace horizon {

ToolHelperDrawNetSetting::ToolHelperDrawNetSetting(IDocument *c, ToolID tid) : ToolBase(c, tid)
{
}

ToolID ToolHelperDrawNetSetting::get_tool_id_for_settings() const
{
    return ToolID::DRAW_NET;
}

void ToolHelperDrawNetSetting::Settings::load_from_json(const json &j)
{
    net_label_size = j.value("net_label_size", 1.5_mm);
}

json ToolHelperDrawNetSetting::Settings::serialize() const
{
    json j;
    j["net_label_size"] = net_label_size;
    return j;
}

void ToolHelperDrawNetSetting::step_net_label_size(bool up)
{
    if (up)
        settings.net_label_size += 0.5_mm;
    else if (settings.net_label_size > 0.5_mm)
        settings.net_label_size -= 0.5_mm;

    apply_settings();
}

void ToolHelperDrawNetSetting::ask_net_label_size()
{
    auto r = imp->dialogs.ask_datum("Enter size", settings.net_label_size);
    if (r.first) {
        settings.net_label_size = std::max(r.second, (int64_t)0.5_mm);
        apply_settings();
    }
}

} // namespace horizon
