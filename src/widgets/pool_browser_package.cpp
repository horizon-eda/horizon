#include "pool_browser_package.hpp"
#include "pool/pool.hpp"
#include <set>

namespace horizon {
PoolBrowserPackage::PoolBrowserPackage(Pool *p) : PoolBrowser(p)
{
    construct();
    name_entry = create_search_entry("Name");
    manufacturer_entry = create_search_entry("Manufacturer");
    tags_entry = create_search_entry("Tags");
    install_pool_item_source_tooltip();
    search();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserPackage::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserPackage::create_columns()
{
    {
        auto col = append_column_with_item_source_cr("Package", list_columns.name, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_expand(true);
        col->set_min_width(200);
    }
    {
        auto col = append_column("Manufacturer", list_columns.manufacturer, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(200);
    }
    treeview->append_column("Pads", list_columns.n_pads);
    {
        auto col = append_column("Tags", list_columns.tags, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(100);
    }
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
}

void PoolBrowserPackage::add_sort_controller_columns()
{
    sort_controller->add_column(0, "packages.name");
    sort_controller->add_column(1, "packages.manufacturer");
}

void PoolBrowserPackage::search()
{
    auto selected_uuid = get_selected();
    treeview->unset_model();
    store->clear();
    Gtk::TreeModel::Row row;
    std::string name_search = name_entry->get_text();
    std::string manufacturer_search = manufacturer_entry->get_text();

    std::istringstream iss(tags_entry->get_text());
    std::set<std::string> tags{std::istream_iterator<std::string>{iss}, std::istream_iterator<std::string>{}};
    std::string query;
    if (tags.size() == 0) {
        query = "SELECT packages.uuid, packages.name, packages.manufacturer,  "
                "packages.n_pads, tags_view.tags, packages.filename, packages.pool_uuid, packages.overridden FROM "
                "packages "
                "LEFT JOIN tags_view ON tags_view.uuid = packages.uuid "
                "WHERE packages.name LIKE ? AND packages.manufacturer LIKE ? "
                "GROUP BY packages.uuid "
                + sort_controller->get_order_by();
    }
    else {
        std::ostringstream qs;
        qs << "SELECT packages.uuid, packages.name, packages.manufacturer, "
              "packages.n_pads, tags_view.tags, packages.filename, packages.pool_uuid, packages.overridden FROM "
              "packages "
              "LEFT JOIN tags ON tags.uuid = packages.uuid "
              "LEFT JOIN tags_view ON tags_view.uuid = packages.uuid "
              "WHERE packages.name LIKE ? "
              "AND packages.manufacturer LIKE ? ";
        qs << "AND (";
        for (const auto &it : tags) {
            qs << "tags.tag LIKE ? OR ";
        }
        qs << "0) GROUP by packages.uuid HAVING count(*) >= $ntags";
        qs << sort_controller->get_order_by();
        query = qs.str();
    }
    SQLite::Query q(pool->db, query);
    q.bind(1, "%" + name_search + "%");
    q.bind(2, "%" + manufacturer_search + "%");
    int i = 0;
    for (const auto &it : tags) {
        q.bind(i + 3, it + "%");
        i++;
    }
    if (tags.size())
        q.bind("$ntags", tags.size());

    if (show_none) {
        row = *(store->append());
        row[list_columns.uuid] = UUID();
        row[list_columns.name] = "none";
        row[list_columns.manufacturer] = "none";
    }

    while (q.step()) {
        row = *(store->append());
        row[list_columns.uuid] = q.get<std::string>(0);
        row[list_columns.name] = q.get<std::string>(1);
        row[list_columns.manufacturer] = q.get<std::string>(2);
        row[list_columns.n_pads] = q.get<int>(3);
        row[list_columns.tags] = q.get<std::string>(4);
        row[list_columns.path] = q.get<std::string>(5);
        row[list_columns.source] = pool_item_source_from_db(q.get<std::string>(6), q.get<int>(7));
    }
    treeview->set_model(store);
    select_uuid(selected_uuid);
    scroll_to_selection();
}

UUID PoolBrowserPackage::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserPackage::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}
} // namespace horizon
