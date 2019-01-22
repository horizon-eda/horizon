#pragma once
#include <gtkmm.h>
#include <map>
#include "core/tool_id.hpp"
#include "imp/action.hpp"

namespace horizon {
class LayerHelpBox : public Gtk::ScrolledWindow {
public:
    LayerHelpBox();
    void set_layer(int layer);

    typedef sigc::signal<void, std::pair<ActionID, ToolID>> type_signal_trigger_action;
    type_signal_trigger_action signal_trigger_action()
    {
        return s_signal_trigger_action;
    }

private:
    Gtk::Label *label = nullptr;
    std::map<int, std::string> help_texts;

    type_signal_trigger_action s_signal_trigger_action;
};

} // namespace horizon
