#include "pool_browser_frame.hpp"
#include "pool/pool.hpp"

namespace horizon {
PoolBrowserFrame::PoolBrowserFrame(Pool *p) : PoolBrowser(p)
{
    construct();
    name_entry = create_search_entry("Name");
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserFrame::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserFrame::create_columns()
{
    append_column_with_item_source_cr("Frame", list_columns.name);
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserFrame::add_sort_controller_columns()
{
    sort_controller->add_column(0, "frames.name");
}

void PoolBrowserFrame::search()
{
    prepare_search();

    std::string name_search = name_entry->get_text();

    std::string query =
            "SELECT frames.uuid, frames.name, frames.filename, frames.pool_uuid, frames.overridden FROM frames WHERE "
            "frames.name LIKE ?"
            + sort_controller->get_order_by();
    SQLite::Query q(pool->db, query);
    q.bind(1, "%" + name_search + "%");

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
            row[list_columns.source] = pool_item_source_from_db(q.get<std::string>(3), q.get<int>(4));
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

UUID PoolBrowserFrame::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserFrame::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}
} // namespace horizon
