#include "pool_browser_decal.hpp"
#include "pool/ipool.hpp"
#include "util/sqlite.hpp"
#include "util/sort_controller.hpp"

namespace horizon {
PoolBrowserDecal::PoolBrowserDecal(IPool &p) : PoolBrowser(p)
{
    construct();
    name_entry = create_search_entry("Name", create_pool_selector());
    focus_widget = name_entry;
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserDecal::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserDecal::create_columns()
{
    append_column_with_item_source_cr("Decal", list_columns.name);
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserDecal::add_sort_controller_columns()
{
    sort_controller->add_column(0, "decals.name");
    sort_controller->add_column(mtime_column, "decals.mtime");
}

void PoolBrowserDecal::search()
{
    prepare_search();

    std::string name_search = name_entry->get_text();

    std::string query =
            "SELECT decals.uuid, decals.name, decals.filename, decals.pool_uuid, decals.last_pool_uuid FROM decals "
            "WHERE "
            "decals.name LIKE ?"
            + get_pool_selector_query() + sort_controller->get_order_by();
    SQLite::Query q(pool.get_db(), query);
    q.bind(1, "%" + name_search + "%");
    bind_pool_selector_query(q);

    Gtk::TreeModel::Row row;
    if (show_none) {
        row = *(store->append());
        row[list_columns.uuid] = UUID();
        row[list_columns.name] = "none";
    }
    try {
        while (q.step()) {
            row = *(store->append());
            row[list_columns.uuid] = q.get<std::string>(0);
            row[list_columns.name] = q.get<std::string>(1);
            row[list_columns.path] = q.get<std::string>(2);
            row[list_columns.source] = pool_item_source_from_db(q, 3, 4);
        }
        set_busy(false);
    }
    catch (SQLite::Error &e) {
        if (e.rc == SQLITE_BUSY) {
            set_busy(true);
        }
        else {
            throw;
        }
    }
    finish_search();
}

UUID PoolBrowserDecal::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserDecal::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}
} // namespace horizon
