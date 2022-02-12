#pragma once
#include <gtkmm.h>
#include <mutex>
#include <list>
#include <thread>
#include <atomic>
#include "util/uuid.hpp"

namespace horizon {


class PlaneUpdateDialog : public Gtk::Dialog {
public:
    PlaneUpdateDialog(Gtk::Window &parent, class Board &brd, class Plane *plane);
    ~PlaneUpdateDialog();
    bool was_cancelled() const
    {
        return cancel;
    }
    bool is_done() const
    {
        return done;
    }

    typedef sigc::signal<void> type_signal_done;
    type_signal_done signal_done()
    {
        return s_signal_done;
    }

private:
    Gtk::Spinner *spinner = nullptr;
    void plane_update_thread(Board &brd, Plane *plane);
    Glib::Dispatcher dispatcher;
    std::atomic_bool cancel = false;
    std::thread thread;
    Gtk::Label *status_label = nullptr;
    std::mutex mutex;
    std::map<UUID, std::string> plane_status;
    std::atomic_bool done = false;

    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(layer);
            Gtk::TreeModelColumnRecord::add(net);
            Gtk::TreeModelColumnRecord::add(status);
            Gtk::TreeModelColumnRecord::add(plane);
        }
        Gtk::TreeModelColumn<Glib::ustring> layer;
        Gtk::TreeModelColumn<Glib::ustring> net;
        Gtk::TreeModelColumn<Glib::ustring> status;
        Gtk::TreeModelColumn<UUID> plane;
    };
    ListColumns list_columns;


    Gtk::TreeView *view;
    Glib::RefPtr<Gtk::ListStore> store;

    type_signal_done s_signal_done;
};
} // namespace horizon
