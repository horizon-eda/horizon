#pragma once
#include <gtkmm.h>

namespace horizon {
enum class InToolActionID;
enum class ToolID;

class InToolKeySequencesPreferencesEditor : public Gtk::Grid {
public:
    InToolKeySequencesPreferencesEditor(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                        class Preferences &prefs);
    static InToolKeySequencesPreferencesEditor *create(Preferences &prefs);

private:
    class Preferences &preferences;
    class InToolKeySequencesPreferences &in_tool_keyseq_preferences;

    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(keys);
            Gtk::TreeModelColumnRecord::add(action);
            Gtk::TreeModelColumnRecord::add(tool);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> keys;
        Gtk::TreeModelColumn<InToolActionID> action;
        Gtk::TreeModelColumn<ToolID> tool;
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
