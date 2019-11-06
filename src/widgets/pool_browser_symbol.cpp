#include "pool_browser_symbol.hpp"
#include "pool/pool.hpp"

namespace horizon {
PoolBrowserSymbol::PoolBrowserSymbol(Pool *p, const UUID &uu) : PoolBrowser(p), unit_uuid(uu)
{
    construct();
    name_entry = create_search_entry("Name");
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserSymbol::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserSymbol::create_columns()
{
    append_column_with_item_source_cr("Symbol", list_columns.name);
    treeview->append_column("Unit", list_columns.unit_name);
    treeview->append_column("Manufacturer", list_columns.unit_manufacturer);
}

void PoolBrowserSymbol::add_sort_controller_columns()
{
    sort_controller->add_column(0, "symbols.name");
    sort_controller->add_column(1, "units.name");
    sort_controller->add_column(2, "units.manufacturer");
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserSymbol::set_unit_uuid(const UUID &uu)
{
    unit_uuid = uu;
    search();
}

void PoolBrowserSymbol::search()
{
    prepare_search();

    std::string name_search = name_entry->get_text();

    std::string query =
            "SELECT symbols.uuid, symbols.name, units.name, "
            "units.manufacturer, symbols.filename, symbols.pool_uuid, symbols.overridden FROM symbols,units WHERE "
            "symbols.unit = units.uuid AND (units.uuid=? OR ?) AND "
            "symbols.name LIKE ?"
            + sort_controller->get_order_by();
    SQLite::Query q(pool->db, query);
    q.bind(1, unit_uuid);
    q.bind(2, unit_uuid == UUID());
    q.bind(3, "%" + name_search + "%");

    Gtk::TreeModel::Row row;
    if (show_none) {
        row = *(store->append());
        row[list_columns.uuid] = UUID();
        row[list_columns.name] = "none";
        row[list_columns.unit_name] = "none";
    }
    try {
        while (q.step()) {
            row = *(store->append());
            row[list_columns.uuid] = q.get<std::string>(0);
            row[list_columns.name] = q.get<std::string>(1);
            row[list_columns.unit_name] = q.get<std::string>(2);
            row[list_columns.unit_manufacturer] = q.get<std::string>(3);
            row[list_columns.path] = q.get<std::string>(4);
            row[list_columns.source] = pool_item_source_from_db(q.get<std::string>(5), q.get<int>(6));
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

UUID PoolBrowserSymbol::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserSymbol::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}
} // namespace horizon
