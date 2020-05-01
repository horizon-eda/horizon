#pragma once
#include <gtkmm.h>

namespace horizon {
enum class ActionID;
enum class ToolID;
class ActionButton : public Gtk::Overlay {
public:
    ActionButton(std::pair<ActionID, ToolID> action, const char *icon_name);

    typedef sigc::signal<void, std::pair<ActionID, ToolID>> type_signal_clicked;
    type_signal_clicked signal_clicked()
    {
        return s_signal_clicked;
    }


private:
    Gtk::Button *button = nullptr;
    type_signal_clicked s_signal_clicked;
};
} // namespace horizon
