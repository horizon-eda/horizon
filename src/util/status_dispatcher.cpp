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
    {
        std::lock_guard<std::mutex> lock(mutex);
        m = msg;
        st = status;
    }

    if (revealer)
        revealer->set_reveal_child(st == StatusDispatcher::Status::BUSY || st == StatusDispatcher::Status::ERROR);
    if (label)
        label->set_text(m);
    if (spinner)
        spinner->property_active() = status == StatusDispatcher::Status::BUSY;
    s_signal_notified.emit(st, m);
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

void StatusDispatcher::reset(const std::string &m)
{
    {
        std::lock_guard<std::mutex> lock(mutex);
        status = Status::BUSY;
        msg = m;
    }
    dispatcher.emit();
}

void StatusDispatcher::set_status(Status st, const std::string &m)
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
    }
    dispatcher.emit();
}
} // namespace horizon
