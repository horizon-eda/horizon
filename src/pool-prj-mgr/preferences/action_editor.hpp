#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"
#include "imp/action.hpp"

namespace horizon {

class ActionEditorBase : public Gtk::Box, public Changeable {
public:
    ActionEditorBase(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x, class Preferences &prefs,
                     const std::string &title);

protected:
    Preferences &preferences;
    void update();
    void set_placeholder_text(const char *t);

private:
    Gtk::ListBox *action_listbox = nullptr;

    virtual std::vector<KeySequence> *maybe_get_keys() = 0;
    virtual std::vector<KeySequence> &get_keys() = 0;
    Gtk::Label *placeholder_label = nullptr;
};

} // namespace horizon
