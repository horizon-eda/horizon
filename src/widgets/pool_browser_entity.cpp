#include "pool_browser_entity.hpp"
#include "pool/ipool.hpp"
#include "util/sqlite.hpp"
#include "tag_entry.hpp"
#include "util/sort_controller.hpp"
#include <set>

namespace horizon {
PoolBrowserEntity::PoolBrowserEntity(IPool &p, const std::string &instance)
    : PoolBrowser(p, TreeViewStateStore::get_prefix(instance, "pool_browser_entity"))
{
    construct();
    name_entry = create_search_entry("Name");
    tag_entry = create_tag_entry("Tags", create_pool_selector());
    focus_widget = name_entry;
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserEntity::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserEntity::create_columns()
{
    {
        auto col = append_column_with_item_source_cr("Entity", list_columns.entity_name, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(150);
        install_column_tooltip(*col, list_columns.entity_name);
    }
    {
        auto col = append_column("Manufacturer", list_columns.entity_manufacturer, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
        install_column_tooltip(*col, list_columns.entity_manufacturer);
    }
    treeview->append_column("Prefix", list_columns.prefix);
    treeview->append_column("Gates", list_columns.n_gates);
    {
        auto col = append_column("Tags", list_columns.tags, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
        install_column_tooltip(*col, list_columns.tags);
    }
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserEntity::add_sort_controller_columns()
{
    sort_controller->add_column(0, "entities.name");
    sort_controller->add_column(1, "entities.manufacturer");
    sort_controller->add_column(mtime_column, "entities.mtime");
}

void PoolBrowserEntity::search()
{
    prepare_search();

    const std::string name_search = name_entry->get_text();

    const auto tags = tag_entry->get_tags();
    std::string query =
            "SELECT entities.uuid, entities.name, entities.prefix, "
            "entities.n_gates, tags_view.tags, entities.manufacturer, "
            "entities.filename, entities.pool_uuid, entities.last_pool_uuid FROM entities "
            "LEFT JOIN tags_view ON tags_view.uuid = entities.uuid AND tags_view.type = 'entity' ";
    query += get_tags_query(tags);
    query += "WHERE entities.name LIKE $name ";
    query += get_pool_selector_query();
    query += sort_controller->get_order_by();
    SQLite::Query q(pool.get_db(), query);
    q.bind("$name", "%" + name_search + "%");
    bind_tags_query(q, tags);
    bind_pool_selector_query(q);

    Gtk::TreeModel::Row row;
    if (show_none) {
        row = *(store->append());
        row[list_columns.uuid] = UUID();
        row[list_columns.entity_name] = "none";
    }
    try {
        while (q.step()) {
            row = *(store->append());
            row[list_columns.uuid] = q.get<std::string>(0);
            row[list_columns.entity_name] = q.get<std::string>(1);
            row[list_columns.prefix] = q.get<std::string>(2);
            row[list_columns.n_gates] = q.get<int>(3);
            row[list_columns.tags] = q.get<std::string>(4);
            row[list_columns.entity_manufacturer] = q.get<std::string>(5);
            row[list_columns.path] = q.get<std::string>(6);
            row[list_columns.source] = pool_item_source_from_db(q, 7, 8);
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

UUID PoolBrowserEntity::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserEntity::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}
} // namespace horizon
