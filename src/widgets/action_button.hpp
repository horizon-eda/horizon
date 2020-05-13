#pragma once
#include <gtkmm.h>
#include "imp/action.hpp"

namespace horizon {

class ActionButtonBase {
public:
    typedef sigc::signal<void, ActionToolID> type_signal_action;
    type_signal_action signal_action()
    {
        return s_signal_action;
    }

    virtual void update_key_sequences();
    virtual void add_action(ActionToolID act) = 0;
    virtual void set_keep_primary_action(bool keep)
    {
    }

    virtual ~ActionButtonBase()
    {
    }

protected:
    ActionButtonBase(const std::map<ActionToolID, ActionConnection> &k);

    type_signal_action s_signal_action;
    std::map<ActionToolID, Gtk::Label *> key_labels;
    const std::map<ActionToolID, ActionConnection> &keys;
};

class ActionButton : public Gtk::Overlay, public ActionButtonBase {
public:
    ActionButton(ActionToolID action, const std::map<ActionToolID, ActionConnection> &k);

    void update_key_sequences() override;
    void add_action(ActionToolID act) override;
    void set_keep_primary_action(bool keep) override;

private:
    const ActionToolID action_orig;
    ActionToolID action;
    Gtk::Button *button = nullptr;
    Gtk::MenuButton *menu_button = nullptr;
    Gtk::Menu menu;
    int button_current = -1;
    Gtk::MenuItem &add_menu_item(ActionToolID act);
    void set_primary_action(ActionToolID act);
    bool keep_primary_action = false;
};

class ActionButtonMenu : public Gtk::Button, public ActionButtonBase {
public:
    ActionButtonMenu(const char *icon_name, const std::map<ActionToolID, ActionConnection> &k);

    void add_action(ActionToolID act) override;

private:
    Gtk::Menu menu;
    int button_current = -1;
    Gtk::MenuItem &add_menu_item(ActionToolID act);
};


} // namespace horizon
