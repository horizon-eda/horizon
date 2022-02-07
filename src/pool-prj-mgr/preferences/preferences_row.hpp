#pragma once
#include <gtkmm.h>
#include "preferences/preferences.hpp"

namespace horizon {

class PreferencesRow : public Gtk::Box {
public:
    PreferencesRow(const std::string &title, const std::string &subtitle, Preferences &prefs);
    virtual void activate()
    {
    }

protected:
    Preferences &preferences;
};


class PreferencesRowBool : public PreferencesRow {
public:
    PreferencesRowBool(const std::string &title, const std::string &subtitle, Preferences &prefs, bool &v);
    void activate() override;

private:
    Gtk::Switch *sw = nullptr;
};

class PreferencesRowBoolButton : public PreferencesRow {
public:
    PreferencesRowBoolButton(const std::string &title, const std::string &subtitle, const std::string &label_true,
                             const std::string &label_false, Preferences &prefs, bool &v);
};

template <typename T> class PreferencesRowNumeric : public PreferencesRow {
public:
    PreferencesRowNumeric(const std::string &title, const std::string &subtitle, Preferences &prefs, T &v)
        : PreferencesRow(title, subtitle, prefs), value(v)
    {
        sp = Gtk::manage(new Gtk::SpinButton);
        sp->set_valign(Gtk::ALIGN_CENTER);
        sp->show();
        pack_start(*sp, false, false, 0);
    }

    Gtk::SpinButton &get_spinbutton()
    {
        return *sp;
    }

    void bind()
    {
        sp->set_value(value);
        sp->signal_value_changed().connect([this] {
            value = sp->get_value();
            preferences.signal_changed().emit();
        });
    }

private:
    T &value;
    Gtk::SpinButton *sp = nullptr;
};

class PreferencesGroup : public Gtk::Box {
public:
    PreferencesGroup(const std::string &title);
    void add_row(PreferencesRow &row);

private:
    Gtk::ListBox *listbox = nullptr;
};

} // namespace horizon
