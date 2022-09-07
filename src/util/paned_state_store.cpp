#include "paned_state_store.hpp"
#include "util.hpp"
#include "sqlite.hpp"
#include "automatic_prefs.hpp"
#include <glibmm/miscutils.h>
#include <gtkmm.h>

namespace horizon {

PanedStateStore::PanedStateStore(Gtk::Paned *pn, const std::string &pr)
    : db(AutomaticPreferences::get().db), prefix("paned-" + pr), paned(pn)
{
    paned->signal_realize().connect(sigc::mem_fun(*this, &PanedStateStore::realize));
}

void PanedStateStore::realize()
{
    SQLite::Query q(db, "SELECT width FROM widths WHERE key = ?");
    q.bind(1, prefix);
    if (q.step()) {
        const int width = q.get<int>(0);
        paned->set_position(width);
    }
    paned->property_position().signal_changed().connect(sigc::track_obj(
            [this] {
                position = paned->get_position();
                timer_connection.disconnect();
                timer_connection =
                        Glib::signal_timeout().connect_seconds(sigc::mem_fun(*this, &PanedStateStore::save), 1);
            },
            *this));
}

bool PanedStateStore::save()
{
    db.execute("BEGIN");
    SQLite::Query q(db, "INSERT or replace into widths VALUES (?,?)");
    q.bind(1, prefix);
    q.bind(2, (int)position);
    q.step();
    db.execute("COMMIT");
    return false;
}

} // namespace horizon
