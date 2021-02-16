#pragma once
#include <string>
#include <map>
#include <sigc++/sigc++.h>

namespace Gtk {
class Paned;
}


namespace horizon {
namespace SQLite {
class Database;
}

class PanedStateStore : public sigc::trackable {
public:
    PanedStateStore(Gtk::Paned *paned, const std::string &prefix);

private:
    SQLite::Database &db;
    const std::string prefix;
    Gtk::Paned *paned = nullptr;
    unsigned int position = 0;
    sigc::connection timer_connection;
    bool save();
    void realize();
};
} // namespace horizon
