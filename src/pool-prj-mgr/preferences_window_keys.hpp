#pragma once
#include <gtkmm.h>
#include "imp/action.hpp"

namespace horizon {

class KeySequencesPreferencesEditor : public Gtk::Grid {
    friend class ActionEditor;

public:
    KeySequencesPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                  class Preferences *prefs, class KeySequencesPreferences *keyseq_prefs);
    static KeySequencesPreferencesEditor *create(Preferences *prefs, KeySequencesPreferences *keyseq_prefs);

private:
    Preferences *preferences;
    KeySequencesPreferences *keyseq_preferences;

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(keys);
            Gtk::TreeModelColumnRecord::add(action);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> keys;
        Gtk::TreeModelColumn<ActionToolID> action;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::TreeStore> key_sequences_store;
    Gtk::TreeView *key_sequences_treeview = nullptr;

    Gtk::FlowBox *action_editors = nullptr;

    void update_action_editors();
    void update_keys();
    void handle_save();
    void handle_load();
    void handle_load_default();
};

} // namespace horizon
