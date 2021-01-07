#include "status_dispatcher.hpp"

namespace horizon {
StatusDispatcher::StatusDispatcher()
{
    dispatcher.connect(sigc::mem_fun(*this, &StatusDispatcher::notify));
}

void StatusDispatcher::notify()
{
    std::string m;
    Status st;
    double p;
    {
        std::lock_guard<std::mutex> lock(mutex);
        m = msg;
        st = status;
        p = progress;
    }

    if (revealer) {
        if (st == StatusDispatcher::Status::BUSY || st == StatusDispatcher::Status::ERROR) {
            timeout_conn.disconnect();
            revealer->set_reveal_child(true);
        }
        else {
            timeout_conn = Glib::signal_timeout().connect(
                    [this] {
                        revealer->set_reveal_child(false);
                        return false;
                    },
                    750);
        }
    }
    if (label)
        label->set_text(m);
    if (spinner)
        spinner->property_active() = status == StatusDispatcher::Status::BUSY;
    if (progress_bar) {
        progress_bar->set_visible(progress >= 0);
        progress_bar->set_fraction(progress);
    }
    Notification n;
    n.status = st;
    n.msg = m;
    n.progress = p;
    s_signal_notified.emit(n);
}

void StatusDispatcher::attach(Gtk::Label *w)
{
    label = w;
}

void StatusDispatcher::attach(Gtk::Spinner *w)
{
    spinner = w;
}

void StatusDispatcher::attach(Gtk::Revealer *w)
{
    revealer = w;
}

void StatusDispatcher::attach(Gtk::ProgressBar *w)
{
    progress_bar = w;
}

void StatusDispatcher::reset(const std::string &m)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        status = Status::BUSY;
        msg = m;
        progress = 0;
    }
    dispatcher.emit();
}

void StatusDispatcher::set_status(Status st, const std::string &m, double p)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        switch (status) {
        case Status::BUSY:
            break;
        case Status::DONE:
            return;
        case Status::ERROR:
            return;
        }
        status = st;
        msg = m;
        progress = p;
    }
    dispatcher.emit();
}
} // namespace horizon
