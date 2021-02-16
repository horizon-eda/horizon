#pragma once
#include <string>
#include <map>
#include <sigc++/sigc++.h>

namespace Gtk {
class TreeView;
}


namespace horizon {
namespace SQLite {
class Database;
}

class TreeViewStateStore : public sigc::trackable {
public:
    TreeViewStateStore(Gtk::TreeView *view, const std::string &prefix);
    static std::string get_prefix(const std::string &instance, const std::string &widget);

private:
    SQLite::Database &db;
    const std::string prefix;
    Gtk::TreeView *view = nullptr;
    std::string get_key(int column) const;
    std::map<unsigned int, unsigned int> column_widths;
    sigc::connection timer_connection;
    bool save();
    void realize();
};
} // namespace horizon
