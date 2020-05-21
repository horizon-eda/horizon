#include "recent_item_box.hpp"

namespace horizon {
RecentItemBox::RecentItemBox(const std::string &name, const std::string &pa, const Glib::DateTime &ti)
    : path(pa), time(ti)
{
    auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6));
    box->property_margin() = 12;
    auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
    {
        auto la = Gtk::manage(new Gtk::Label());
        la->set_markup("<b>" + Glib::Markup::escape_text(name) + "</b>");
        la->set_xalign(0);
        tbox->pack_start(*la, true, true, 0);
    }
    {
        time_label = Gtk::manage(new Gtk::Label());
        time_label->set_tooltip_text(time.format("%c"));
        tbox->pack_start(*time_label, false, false, 0);
        update_time();
    }
    box->pack_start(*tbox, true, true, 0);
    {
        auto la = Gtk::manage(new Gtk::Label(path));
        la->set_xalign(0);
        la->set_ellipsize(Pango::ELLIPSIZE_START);
        la->set_tooltip_text(path);
        la->get_style_context()->add_class("dim-label");
        box->pack_start(*la, false, false, 0);
    }

    {
        auto item = Gtk::manage(new Gtk::MenuItem("Remove from this list"));
        item->signal_activate().connect([this] { s_signal_remove.emit(); });
        item->show();
        menu.append(*item);
    }
    {
        auto item = Gtk::manage(new Gtk::MenuItem("Open in file browser"));
        item->signal_activate().connect([this] {
            auto uri = Gio::File::create_for_path(Glib::path_get_dirname(path))->get_uri();
            Gio::AppInfo::launch_default_for_uri(uri);
        });
        item->show();
        menu.append(*item);
    }

    Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(*this, &RecentItemBox::update_time), true),
                                           1);
    add_events(Gdk::BUTTON_PRESS_MASK);
    signal_button_press_event().connect([this](GdkEventButton *ev) {
        if (gdk_event_triggers_context_menu((GdkEvent *)ev)) {
            menu.popup_at_pointer((GdkEvent *)ev);
            return true;
        }
        else {
            return false;
        }
    });
    add(*box);
    show_all();
}

void RecentItemBox::update_time()
{
    auto now = Glib::DateTime::create_now_local();
    auto delta_sec = now.to_unix() - time.to_unix();
    auto one_year = (60 * 60 * 24 * 356);
    auto one_month = (60 * 60 * 24 * 30);
    auto one_week = (60 * 60 * 24 * 7);
    auto one_day = (60 * 60 * 24);
    auto one_hour = (60 * 60);
    auto one_minute = (60);
    int n = 0;
    std::string unit;
    bool is_hour = false;
    if (delta_sec >= one_year) {
        n = delta_sec / one_year;
        unit = "year";
    }
    else if (delta_sec >= one_month) {
        n = delta_sec / one_month;
        unit = "month";
    }
    else if (delta_sec >= one_week) {
        n = delta_sec / one_week;
        unit = "week";
    }
    else if (delta_sec >= one_day) {
        n = delta_sec / one_day;
        unit = "day";
    }
    else if (delta_sec >= one_hour) {
        n = delta_sec / one_hour;
        unit = "hour";
        is_hour = true;
    }
    else if (delta_sec >= one_minute) {
        n = delta_sec / one_minute;
        unit = "minute";
    }

    std::string s;
    if (n == 0) {
        s = "just now";
    }
    else {
        if (is_hour && n == 1) {
            s = "an";
        }
        else if (n == 1) {
            s = "a";
        }
        else {
            s = std::to_string(n);
        }
        s += " " + unit;
        if (n > 1)
            s += "s";
        s += " ago";
    }

    time_label->set_text(s);
}
} // namespace horizon
