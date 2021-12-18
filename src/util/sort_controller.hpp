#pragma once
#include <gtkmm.h>
#include <sigc++/sigc++.h>

namespace horizon {
class SortController : public sigc::trackable {
public:
    enum class Sort { ASC, DESC, NONE };

    SortController(Gtk::TreeView *tv);
    void add_column(int index, const std::string &name);
    void set_simple(bool s);
    std::string get_order_by() const;
    void set_sort(int index, Sort s);

    typedef sigc::signal<void> type_signal_changed;
    type_signal_changed signal_changed()
    {
        return s_signal_changed;
    }

private:
    Gtk::TreeView *treeview;
    std::map<int, std::pair<std::string, Sort>> columns;
    void update_treeview();
    void handle_click(int index);
    bool is_simple;

    type_signal_changed s_signal_changed;
};
} // namespace horizon
