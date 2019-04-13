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
    void set_status(Status status, const std::string &msg, double progress = 0);

    class Notification {
    public:
        Status status;
        std::string msg;
        double progress;
    };

    typedef sigc::signal<void, const Notification &> type_signal_notified;
    type_signal_notified signal_notified()
    {
        return s_signal_notified;
    }

    void attach(Gtk::Spinner *w);
    void attach(Gtk::Label *w);
    void attach(Gtk::Revealer *w);
    void attach(Gtk::ProgressBar *w);

private:
    void notify();
    Glib::Dispatcher dispatcher;
    std::mutex mutex;
    std::string msg;
    double progress = 0;
    Status status = Status::DONE;

    type_signal_notified s_signal_notified;
    sigc::connection timeout_conn;

    Gtk::Spinner *spinner = nullptr;
    Gtk::Label *label = nullptr;
    Gtk::Revealer *revealer = nullptr;
    Gtk::ProgressBar *progress_bar = nullptr;
};
} // namespace horizon
