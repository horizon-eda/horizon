#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"
#include <set>

namespace horizon {
class ConfirmCloseDialog : public Gtk::MessageDialog {
public:
    ConfirmCloseDialog(Gtk::Window *parent);
    void set_files(std::map<std::string, std::map<UUID, std::string>> &files);
    std::map<std::string, std::set<UUID>> get_files() const;

    enum {
        RESPONSE_SAVE = 1,
        RESPONSE_NO_SAVE = 2,
    };

private:
    class TreeColumns : public Gtk::TreeModelColumnRecord {
    public:
        TreeColumns()
        {
            Gtk::TreeModelColumnRecord::add(name);
            Gtk::TreeModelColumnRecord::add(display_name);
            Gtk::TreeModelColumnRecord::add(uuid);
            Gtk::TreeModelColumnRecord::add(save);
            Gtk::TreeModelColumnRecord::add(inconsistent);
        }
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<Glib::ustring> display_name;
        Gtk::TreeModelColumn<UUID> uuid;
        Gtk::TreeModelColumn<bool> save;
        Gtk::TreeModelColumn<bool> inconsistent;
    };
    TreeColumns tree_columns;

    Glib::RefPtr<Gtk::TreeStore> store;

    Gtk::TreeView *tv = nullptr;
};

class ProcWaitDialog : public Gtk::Dialog {
public:
    ProcWaitDialog(class PoolProjectManagerAppWindow *parent);
};
} // namespace horizon
