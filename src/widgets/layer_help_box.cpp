#include "layer_help_box.hpp"
#include "util/util.hpp"
#include "util/str_util.hpp"
#include "nlohmann/json.hpp"
#include "imp/action_catalog.hpp"
#include "pool/pool.hpp"
#include "pool/pool_manager.hpp"
#include <iostream>

namespace horizon {
LayerHelpBox::LayerHelpBox(class Pool &pool) : Gtk::ScrolledWindow()
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

    auto base_path = pool.get_base_path();
    load(base_path);
    auto pools = PoolManager::get().get_pools();
    if (pools.count(base_path)) {
        const auto &pools_included = pools.at(base_path).pools_included;
        for (auto it = pools_included.crbegin(); it != pools_included.crend(); it++) {
            auto other_pool = PoolManager::get().get_by_uuid(*it);
            if (other_pool) {
                load(other_pool->base_path);
            }
        }
    }
}

void LayerHelpBox::load(const std::string &pool_path)
{
    auto path = Glib::build_filename(pool_path, "layer_help");
    if (!Glib::file_test(path, Glib::FILE_TEST_IS_DIR))
        return;
    Glib::Dir dir(path);
    for (const auto &l : dir) {
        auto p = Glib::build_filename(path, l);
        std::ifstream ifs(p);
        if (!ifs.is_open()) {
            continue;
        }
        std::string text;
        ifs >> text;
        help_texts.emplace(std::stoi(l), text);
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
