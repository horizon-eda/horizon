#include "layer_help_box.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "nlohmann/json.hpp"
#include "imp/action_catalog.hpp"
#include <iostream>

namespace horizon {
LayerHelpBox::LayerHelpBox() : Gtk::ScrolledWindow()
{
    label = Gtk::manage(new Gtk::Label());
    label->set_text("fixme");
    label->show();
    label->set_xalign(0);
    label->set_valign(Gtk::ALIGN_START);
    label->set_line_wrap(true);
    label->set_line_wrap_mode(Pango::WRAP_WORD);
    add(*label);
    set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);

    label->set_track_visited_links(false);
    label->signal_activate_link().connect(
            [this](const std::string &url) {
                if (url.find("a:") == 0) {
                    std::string action = url.substr(2);
                    std::cout << "act " << action << std::endl;
                    s_signal_trigger_action.emit(make_action(action_lut.lookup(action)));
                }
                else if (url.find("t:") == 0) {
                    std::string tool = url.substr(2);
                    std::cout << "tool " << tool << std::endl;
                    s_signal_trigger_action.emit(make_action(tool_lut.lookup(tool)));
                }
                return true;
            },
            false);

    const auto j = json_from_resource("/net/carrotIndustries/horizon/imp/layer_help.json");
    for (auto it = j.cbegin(); it != j.cend(); ++it) {
        help_texts.emplace(std::stoi(it.key()), it.value());
    }
}

void LayerHelpBox::set_layer(int layer)
{
    if (help_texts.count(layer))
        label->set_markup(help_texts.at(layer));
    else
        label->set_markup("");
}
} // namespace horizon
