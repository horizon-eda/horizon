#pragma once
#include <glibmm/dispatcher.h>
#include <mutex>
#include <string>
#include <gtkmm.h>

namespace horizon {
class StatusDispatcher : public sigc::trackable {
public:
    StatusDispatcher();

    enum class Status { BUSY, DONE, ERROR };

    void reset(const std::string &msg);
    void set_status(Status status, const std::string &msg);

    typedef sigc::signal<void, Status, std::string> type_signal_notified;
    type_signal_notified signal_notified()
    {
        return s_signal_notified;
    }

    void attach(Gtk::Spinner *w);
    void attach(Gtk::Label *w);
    void attach(Gtk::Revealer *w);

private:
    void notify();
    Glib::Dispatcher dispatcher;
    std::mutex mutex;
    std::string msg;
    Status status = Status::DONE;

    type_signal_notified s_signal_notified;

    Gtk::Spinner *spinner = nullptr;
    Gtk::Label *label = nullptr;
    Gtk::Revealer *revealer = nullptr;
};
} // namespace horizon
