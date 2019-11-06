#include "pool_browser_entity.hpp"
#include "pool/pool.hpp"
#include "tag_entry.hpp"
#include <set>

namespace horizon {
PoolBrowserEntity::PoolBrowserEntity(Pool *p) : PoolBrowser(p)
{
    construct();
    name_entry = create_search_entry("Name");
    tag_entry = create_tag_entry("Tags");
    install_pool_item_source_tooltip();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserEntity::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserEntity::create_columns()
{
    append_column_with_item_source_cr("Entity", list_columns.entity_name);
    treeview->append_column("Manufacturer", list_columns.entity_manufacturer);
    treeview->append_column("Prefix", list_columns.prefix);
    treeview->append_column("Gates", list_columns.n_gates);
    {
        auto col = append_column("Tags", list_columns.tags);
        install_column_tooltip(*col, list_columns.tags);
    }
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserEntity::add_sort_controller_columns()
{
    sort_controller->add_column(0, "entities.name");
    sort_controller->add_column(1, "entities.manufacturer");
}

void PoolBrowserEntity::search()
{
    prepare_search();

    std::string name_search = name_entry->get_text();

    auto tags = tag_entry->get_tags();
    std::string query;
    if (tags.size() == 0) {
        query = "SELECT entities.uuid, entities.name, entities.prefix, "
                "entities.n_gates, tags_view.tags, "
                "entities.manufacturer, entities.filename, entities.pool_uuid, entities.overridden FROM entities "
                "LEFT JOIN tags_view ON tags_view.uuid = entities.uuid AND tags_view.type = 'entity' "
                "WHERE entities.name LIKE $name "
                + sort_controller->get_order_by();
    }
    else {
        std::ostringstream qs;
        qs << "SELECT entities.uuid, entities.name, entities.prefix, "
              "entities.n_gates, tags_view.tags, entities.manufacturer, "
              "entities.filename, entities.pool_uuid, entities.overridden FROM entities "
              "LEFT JOIN tags_view ON tags_view.uuid = entities.uuid AND tags_view.type = 'entity' "
              "INNER JOIN (SELECT uuid FROM tags WHERE tags.tag IN (";

        int i = 0;
        for (const auto &it : tags) {
            (void)sizeof it;
            qs << "$tag" << i << ", ";
            i++;
        }
        qs << "'') AND tags.type = 'entity' "
              "GROUP by tags.uuid HAVING count(*) >= $ntags) as x ON x.uuid = entities.uuid "
              "WHERE entities.name LIKE $name ";

        qs << sort_controller->get_order_by();
        query = qs.str();
    }
    std::cout << query << std::endl;
    SQLite::Query q(pool->db, query);
    q.bind("$name", "%" + name_search + "%");
    int i = 0;
    for (const auto &it : tags) {
        q.bind(("$tag" + std::to_string(i)).c_str(), it);
        i++;
    }
    if (tags.size())
        q.bind("$ntags", tags.size());

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
            row[list_columns.source] = pool_item_source_from_db(q.get<std::string>(7), q.get<int>(8));
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
