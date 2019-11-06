#include "pool_browser_unit.hpp"
#include "pool/pool.hpp"
#include "widgets/cell_renderer_color_box.hpp"

namespace horizon {
PoolBrowserUnit::PoolBrowserUnit(Pool *p) : PoolBrowser(p)
{
    construct();
    name_entry = create_search_entry("Name");
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserUnit::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserUnit::create_columns()
{
    append_column_with_item_source_cr("Unit", list_columns.name);
    treeview->append_column("Manufacturer", list_columns.manufacturer);
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserUnit::add_sort_controller_columns()
{
    sort_controller->add_column(0, "units.name");
    sort_controller->add_column(1, "units.manufacturer");
}

void PoolBrowserUnit::search()
{
    prepare_search();

    std::string name_search = name_entry->get_text();

    std::string query =
            "SELECT units.uuid, units.name, units.manufacturer, units.filename, units.pool_uuid, units.overridden "
            "FROM units WHERE units.name LIKE ?"
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
            row[list_columns.manufacturer] = q.get<std::string>(2);
            row[list_columns.path] = q.get<std::string>(3);
            row[list_columns.source] = pool_item_source_from_db(q.get<std::string>(4), q.get<int>(5));
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

UUID PoolBrowserUnit::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserUnit::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}

} // namespace horizon
