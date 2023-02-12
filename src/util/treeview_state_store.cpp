#include "treeview_state_store.hpp"
#include "util.hpp"
#include "sqlite.hpp"
#include "automatic_prefs.hpp"
#include <glibmm/miscutils.h>
#include <gtkmm.h>

namespace horizon {

TreeViewStateStore::TreeViewStateStore(Gtk::TreeView *tv, const std::string &pr)
    : db(AutomaticPreferences::get().db), prefix("treeview-" + pr + "-"), view(tv)
{
    view->signal_realize().connect(sigc::mem_fun(*this, &TreeViewStateStore::realize));
}

void TreeViewStateStore::realize()
{
    const int n_col = view->get_n_columns();
    // don't attach to last visible column
    int last_visible = -1;
    for (int i = 0; i < n_col; i++) {
        auto col = view->get_column(i);
        if (col->get_visible() && col->get_resizable())
            last_visible = i;
    }
    for (int i = 0; i < n_col; i++) {
        auto col = view->get_column(i);
        if (col->get_resizable() && i != last_visible) {
            SQLite::Query q(db, "SELECT width FROM widths WHERE key = ?");
            q.bind(1, get_key(i));
            if (q.step()) {
                const int width = q.get<int>(0);
                col->set_sizing(Gtk::TREE_VIEW_COLUMN_FIXED);
                col->set_fixed_width(width);
            }
            col->property_width().signal_changed().connect(sigc::track_obj(
                    [this, i] {
                        column_widths[i] = view->get_column(i)->get_width();
                        timer_connection.disconnect();
                        timer_connection = Glib::signal_timeout().connect_seconds(
                                sigc::mem_fun(*this, &TreeViewStateStore::save), 1);
                    },
                    *this));
        }
    }
}

std::string TreeViewStateStore::get_key(int column) const
{
    return prefix + std::to_string(column);
}


std::string TreeViewStateStore::get_prefix(const std::string &instance, const std::string &widget)
{
    if (instance.size())
        return instance + "-" + widget;
    else
        return "";
}

bool TreeViewStateStore::save()
{
    db.execute("BEGIN");
    for (const auto &[col, width] : column_widths) {
        SQLite::Query q(db, "INSERT or replace into widths VALUES (?,?)");
        q.bind(1, get_key(col));
        q.bind(2, width);
        q.step();
    }
    db.execute("COMMIT");
    column_widths.clear();
    return false;
}

} // namespace horizon
