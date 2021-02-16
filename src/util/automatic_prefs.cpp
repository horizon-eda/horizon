#include "automatic_prefs.hpp"
#include "util.hpp"
#include <glibmm/miscutils.h>
#include <glibmm/fileutils.h>
#include <giomm/resource.h>
#include <giomm/file.h>

namespace horizon {
AutomaticPreferences &AutomaticPreferences::get()
{
    static auto x = AutomaticPreferences();
    return x;
}

static const int min_user_version = 2;

AutomaticPreferences::AutomaticPreferences()
    : db_path(Glib::build_filename(get_config_dir(), "automatic_prefs.db")),
      db(db_path, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, 1000)
{
    {
        int user_version = db.get_user_version();
        if (user_version < min_user_version) {
            // update schema
            auto bytes = Gio::Resource::lookup_data_global(
                    "/org/horizon-eda/horizon/util/"
                    "automatic_prefs_schema.sql");
            gsize size{bytes->get_size() + 1}; // null byte
            auto data = (const char *)bytes->get_data(size);
            db.execute(data);
            auto old_db_path = Glib::build_filename(get_config_dir(), "window_state.db");
            if (user_version == 0 && Glib::file_test(old_db_path, Glib::FILE_TEST_IS_REGULAR)) {
                {
                    SQLite::Query q(db, "ATTACH ? AS old");
                    q.bind(1, old_db_path);
                    q.step();
                }
                db.execute("INSERT INTO window_state SELECT * FROM old.window_state");
                db.execute("DETACH old");
            }
        }
    }
}


} // namespace horizon
