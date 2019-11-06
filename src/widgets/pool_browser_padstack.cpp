#include "pool_browser_padstack.hpp"
#include "pool/pool.hpp"
#include <set>

namespace horizon {
PoolBrowserPadstack::PoolBrowserPadstack(Pool *p) : PoolBrowser(p)
{
    construct();
    name_entry = create_search_entry("Name");
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserPadstack::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserPadstack::create_columns()
{
    append_column_with_item_source_cr("Padstack", list_columns.padstack_name);
    treeview->append_column("Type", list_columns.padstack_type);
    treeview->append_column("Package", list_columns.package_name);
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserPadstack::add_sort_controller_columns()
{
    sort_controller->add_column(0, "padstacks.name");
}

void PoolBrowserPadstack::set_package_uuid(const UUID &uu)
{
    package_uuid = uu;
    search();
}

void PoolBrowserPadstack::set_include_padstack_type(Padstack::Type ty, bool v)
{
    if (padstacks_included.count(ty)) {
        if (!v)
            padstacks_included.erase(ty);
    }
    if (v)
        padstacks_included.insert(ty);
    search();
}

void PoolBrowserPadstack::search()
{
    prepare_search();

    std::string name_search = name_entry->get_text();

    SQLite::Query q(pool->db,
                    "SELECT padstacks.uuid, padstacks.name, padstacks.type, "
                    "packages.name, padstacks.filename, padstacks.pool_uuid, padstacks.overridden FROM padstacks LEFT "
                    "JOIN packages ON padstacks.package = packages.uuid WHERE "
                    "(packages.uuid=? OR ? OR padstacks.package = "
                    "'00000000-0000-0000-0000-000000000000')  AND "
                    "padstacks.name LIKE ? AND padstacks.type IN (?, ?, ?, ?, "
                    "?, ?) " + sort_controller->get_order_by());
    q.bind(1, package_uuid);
    q.bind(2, package_uuid == UUID());
    q.bind(3, "%" + name_search + "%");
    if (padstacks_included.count(Padstack::Type::TOP))
        q.bind(4, std::string("top"));
    else
        q.bind(4, std::string(""));

    if (padstacks_included.count(Padstack::Type::BOTTOM))
        q.bind(5, std::string("bottom"));
    else
        q.bind(5, std::string(""));

    if (padstacks_included.count(Padstack::Type::THROUGH))
        q.bind(6, std::string("through"));
    else
        q.bind(6, std::string(""));

    if (padstacks_included.count(Padstack::Type::MECHANICAL))
        q.bind(7, std::string("mechanical"));
    else
        q.bind(7, std::string(""));

    if (padstacks_included.count(Padstack::Type::VIA))
        q.bind(8, std::string("via"));
    else
        q.bind(8, std::string(""));

    if (padstacks_included.count(Padstack::Type::HOLE))
        q.bind(9, std::string("hole"));
    else
        q.bind(9, std::string(""));


    Gtk::TreeModel::Row row;
    if (show_none) {
        row = *(store->append());
        row[list_columns.uuid] = UUID();
        row[list_columns.padstack_name] = "none";
    }
    try {
        while (q.step()) {
            row = *(store->append());
            row[list_columns.uuid] = q.get<std::string>(0);
            row[list_columns.padstack_name] = q.get<std::string>(1);
            row[list_columns.padstack_type] = q.get<std::string>(2);
            row[list_columns.package_name] = q.get<std::string>(3);
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

UUID PoolBrowserPadstack::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserPadstack::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}
} // namespace horizon
