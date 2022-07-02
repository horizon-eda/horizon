#include "sort_helper.hpp"

namespace horizon {
void SortHelper::attach(const std::string &column, Gtk::Button &button, Gtk::Image &indicator)
{
    std::string col = column;
    button.signal_clicked().connect([this, col] { handle_button(col); });
    columns.push_back({column, indicator});
}

void SortHelper::attach(const std::string &column, const Glib::RefPtr<Gtk::Builder> &x)
{
    Gtk::Button *button;
    Gtk::Image *img;
    x->get_widget(column + "_button", button);
    x->get_widget(column + "_sort_indicator", img);
    attach(column, *button, *img);
}

void SortHelper::handle_button(const std::string &col)
{
    if (sort_column == col) {
        if (sort_type == Gtk::SORT_ASCENDING)
            sort_type = Gtk::SORT_DESCENDING;
        else
            sort_type = Gtk::SORT_ASCENDING;
    }
    else {
        sort_column = col;
        sort_type = Gtk::SORT_ASCENDING;
    }
    update();
    s_signal_changed.emit();
}

void SortHelper::set_sort(const std::string &col, Gtk::SortType st)
{
    sort_column = col;
    sort_type = st;
    update();
    s_signal_changed.emit();
}

const std::string &SortHelper::get_column() const
{
    return sort_column;
}

Gtk::SortType SortHelper::get_sort_type() const
{
    return sort_type;
}

void SortHelper::update()
{
    for (auto &col : columns) {
        if (col.name == sort_column) {
            col.indicator.show();
            if (sort_type == Gtk::SORT_ASCENDING)
                col.indicator.set_from_icon_name("pan-down-symbolic", Gtk::ICON_SIZE_BUTTON);
            else
                col.indicator.set_from_icon_name("pan-up-symbolic", Gtk::ICON_SIZE_BUTTON);
        }
        else {
            col.indicator.hide();
        }
    }
}

int SortHelper::transform_order(int x) const
{
    if (sort_type == Gtk::SORT_ASCENDING)
        return x;
    else
        return -x;
}
} // namespace horizon
