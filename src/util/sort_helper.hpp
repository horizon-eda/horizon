#pragma once
#include <gtkmm.h>
#include "util/changeable.hpp"

namespace horizon {
class SortHelper : public Changeable, public sigc::trackable {
public:
    const std::string &get_column() const;
    Gtk::SortType get_sort_type() const;
    int transform_order(int x) const;
    void attach(const std::string &column, Gtk::Button &button, Gtk::Image &indicator);
    void attach(const std::string &column, const Glib::RefPtr<Gtk::Builder> &x);
    void set_sort(const std::string &col, Gtk::SortType st);

private:
    void handle_button(const std::string &col);
    void update();
    struct Column {
        const std::string name;
        Gtk::Image &indicator;
    };
    std::vector<Column> columns;
    std::string sort_column;
    Gtk::SortType sort_type;
};
} // namespace horizon
