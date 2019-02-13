#include "recent_item_box.hpp"

namespace horizon {
RecentItemBox::RecentItemBox(const std::string &name, const std::string &pa, const Glib::DateTime &ti)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 6), path(pa), time(ti)
{
    property_margin() = 12;
    auto tbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 12));
    {
        auto la = Gtk::manage(new Gtk::Label());
        la->set_markup("<b>" + name + "</b>");
        la->set_xalign(0);
        tbox->pack_start(*la, true, true, 0);
    }
    {
        time_label = Gtk::manage(new Gtk::Label());
        time_label->set_tooltip_text(time.format("%c"));
        tbox->pack_start(*time_label, false, false, 0);
        update_time();
    }
    pack_start(*tbox, true, true, 0);
    {
        auto la = Gtk::manage(new Gtk::Label(path));
        la->set_xalign(0);
        la->get_style_context()->add_class("dim-label");
        pack_start(*la, false, false, 0);
    }

    Glib::signal_timeout().connect_seconds(sigc::bind_return(sigc::mem_fun(this, &RecentItemBox::update_time), true),
                                           1);

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
        if (n == 1) {
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
