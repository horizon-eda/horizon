#pragma once
#include <gtkmm.h>
#include "action.hpp"

namespace horizon {

class KeySequencesPreferencesEditor : public Gtk::Grid {
    friend class ActionEditor;

public:
    KeySequencesPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                  class ImpPreferences *prefs, class KeySequencesPreferences *keyseq_prefs);
    static KeySequencesPreferencesEditor *create(ImpPreferences *prefs, KeySequencesPreferences *keyseq_prefs);

private:
    ImpPreferences *preferences;
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
        Gtk::TreeModelColumn<std::pair<ActionID, ToolID>> action;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::TreeStore> key_sequences_store;
    Gtk::TreeView *key_sequences_treeview = nullptr;

    Gtk::Box *action_editors = nullptr;

    void update_action_editors();
    void update_keys();
};

} // namespace horizon
