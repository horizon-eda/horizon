#pragma once
#include "action.hpp"
#include "action_catalog.hpp"
#include <gtkmm.h>

namespace horizon {

class ToolPopover : public Gtk::Popover {
public:
    ToolPopover(Gtk::Widget *parent, ActionCatalogItem::Availability av);
    typedef sigc::signal<void, ActionID, ToolID> type_signal_action_activated;
    type_signal_action_activated signal_action_activated()
    {
        return s_signal_action_activated;
    }
    void set_can_begin(const std::map<ActionToolID, bool> &can_begin);
    void set_key_sequences(ActionToolID action_id, const std::vector<KeySequence> &seqs);

private:
    Gtk::SearchEntry *search_entry;
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(action_id);
            Gtk::TreeModelColumnRecord::add(tool_id);
            Gtk::TreeModelColumnRecord::add(can_begin);
            Gtk::TreeModelColumnRecord::add(keys);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ActionID> action_id;
        Gtk::TreeModelColumn<ToolID> tool_id;
        Gtk::TreeModelColumn<bool> can_begin;
        Gtk::TreeModelColumn<Glib::ustring> keys;
    };
    ListColumns list_columns;

    class ListColumnsGroup : public Gtk::TreeModelColumnRecord {
    public:
        ListColumnsGroup()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(group);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<ActionGroup> group;
    };
    ListColumnsGroup list_columns_group;
    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;
    Glib::RefPtr<Gtk::TreeModelFilter> store_filtered;


    Gtk::TreeView *view_group;
    Glib::RefPtr<Gtk::ListStore> store_group;
    Gtk::Revealer *revealer = nullptr;

    void emit_tool_activated();
    type_signal_action_activated s_signal_action_activated;
    void on_show() override;
    std::unique_ptr<Glib::PatternSpec> pattern;
    ActionGroup selected_group = ActionGroup::ALL;
    double y_start = NAN;
    Gtk::ScrolledWindow *sc = nullptr;
    int sc_height = 0;
};
} // namespace horizon
