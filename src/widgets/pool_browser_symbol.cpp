#include "pool_browser_symbol.hpp"
#include "pool/ipool.hpp"
#include "util/sqlite.hpp"
#include "util/sort_controller.hpp"

namespace horizon {
PoolBrowserSymbol::PoolBrowserSymbol(IPool &p, const UUID &uu, const std::string &instance)
    : PoolBrowser(p, TreeViewStateStore::get_prefix(instance, "pool_browser_package")), unit_uuid(uu)
{
    construct();
    name_entry = create_search_entry("Name", create_pool_selector());
    focus_widget = name_entry;
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserSymbol::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserSymbol::create_columns()
{
    {
        auto col = append_column_with_item_source_cr("Symbol", list_columns.name, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(150);
    }
    {
        auto col = append_column("Unit", list_columns.unit_name, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
    }
    {
        auto col = append_column("Manufacturer", list_columns.unit_manufacturer, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
    }
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserSymbol::add_sort_controller_columns()
{
    sort_controller->add_column(0, "symbols.name");
    sort_controller->add_column(1, "units.name");
    sort_controller->add_column(2, "units.manufacturer");
    sort_controller->add_column(mtime_column, "symbols.mtime");
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
            "units.manufacturer, symbols.filename, symbols.pool_uuid, symbols.last_pool_uuid FROM symbols,units WHERE "
            "symbols.unit = units.uuid AND (units.uuid=? OR ?) AND "
            "symbols.name LIKE ?"
            + get_pool_selector_query() + sort_controller->get_order_by();
    SQLite::Query q(pool.get_db(), query);
    q.bind(1, unit_uuid);
    q.bind(2, unit_uuid == UUID());
    q.bind(3, "%" + name_search + "%");
    bind_pool_selector_query(q);

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
            row[list_columns.source] = pool_item_source_from_db(q, 5, 6);
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
