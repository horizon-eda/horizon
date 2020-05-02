#pragma once
#include <gtkmm.h>
#include <map>
#include "core/tool_id.hpp"
#include "imp/action.hpp"

namespace horizon {
class LayerHelpBox : public Gtk::ScrolledWindow {
public:
    LayerHelpBox(class Pool &p);
    void set_layer(int layer);

    typedef sigc::signal<void, ActionToolID> type_signal_trigger_action;
    type_signal_trigger_action signal_trigger_action()
    {
        return s_signal_trigger_action;
    }

private:
    void load(const std::string &path);
    Gtk::Label *label = nullptr;
    std::map<int, std::string> help_texts;

    type_signal_trigger_action s_signal_trigger_action;
};

} // namespace horizon
