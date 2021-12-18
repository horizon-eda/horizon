#include "pool_browser_part.hpp"
#include "pool/ipool.hpp"
#include "util/sqlite.hpp"
#include "pool/part.hpp"
#include "widgets/tag_entry.hpp"
#include "util/sort_controller.hpp"
#include <set>

namespace horizon {
PoolBrowserPart::PoolBrowserPart(IPool &p, const UUID &uu, const std::string &instance)
    : PoolBrowserStockinfo(p, TreeViewStateStore::get_prefix(instance, "pool_browser_part")), entity_uuid(uu)
{
    construct();
    MPN_entry = create_search_entry("MPN");
    focus_widget = MPN_entry;
    manufacturer_entry = create_search_entry("Manufacturer");
    desc_entry = create_search_entry("Description");

    auto extra_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));

    base_parts_cb = Gtk::manage(new Gtk::CheckButton("Base parts"));
    base_parts_cb->set_tooltip_text("Include base parts");
    base_parts_cb->set_active(true);
    base_parts_cb->show();
    base_parts_cb->signal_toggled().connect([this] {
        if (searched_once)
            search();
    });
    extra_box->pack_start(*base_parts_cb, false, false, 0);

    if (auto sel = create_pool_selector()) {
        extra_box->pack_start(*sel, false, false, 0);
        sel->show();
    }
    extra_box->show();

    tag_entry = create_tag_entry("Tags", extra_box);

    install_pool_item_source_tooltip();
}

void PoolBrowserPart::set_MPN(const std::string &s)
{
    MPN_entry->set_text(s);
    search();
}

void PoolBrowserPart::set_entity_uuid(const UUID &uu)
{
    entity_uuid = uu;
    search();
}

void PoolBrowserPart::set_include_base_parts(bool v)
{
    base_parts_cb->set_active(v);
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserPart::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}

void PoolBrowserPart::create_columns()
{
    {
        auto col = append_column_with_item_source_cr("MPN", list_columns.MPN, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
    }
    {
        auto col = append_column("Manufacturer", list_columns.manufacturer, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
    }
    {
        auto col = append_column("Description", list_columns.description, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(100);
        install_column_tooltip(*col, list_columns.description);
    }
    {
        auto col = append_column("Package", list_columns.package, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
    }
    {
        auto col = append_column("Tags", list_columns.tags, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(50);
        install_column_tooltip(*col, list_columns.tags);
    }
    path_column = append_column("Path", list_columns.path, Pango::ELLIPSIZE_START);
    install_column_tooltip(*path_column, list_columns.path);
}

void PoolBrowserPart::add_sort_controller_columns()
{
    sort_controller->add_column(0, "parts.MPN");
    sort_controller->add_column(1, "parts.manufacturer");
    sort_controller->add_column(2, "packages.name");
    sort_controller->add_column(mtime_column, "parts.mtime");
}

void PoolBrowserPart::search()
{
    prepare_search();
    iter_cache.clear();
    Gtk::TreeModel::Row row;
    const std::string MPN_search = MPN_entry->get_text();
    const std::string manufacturer_search = manufacturer_entry->get_text();
    const std::string desc_search = desc_entry->get_text();

    const auto tags = tag_entry->get_tags();
    std::string query =
            "SELECT parts.uuid, parts.MPN, parts.manufacturer, "
            "packages.name, tags_view.tags, parts.filename, "
            "parts.description, parts.pool_uuid, parts.last_pool_uuid "
            "FROM parts LEFT JOIN tags_view ON "
            "tags_view.uuid = parts.uuid AND tags_view.type = 'part' "
            "LEFT JOIN packages ON packages.uuid = parts.package ";
    query += get_tags_query(tags);
    query += "WHERE parts.MPN LIKE $mpn "
             "AND parts.manufacturer LIKE $manufacturer "
             "AND parts.description LIKE $desc "
             "AND (parts.entity=$entity or $entity_all) ";
    if (base_parts_cb->get_active() == false) {
        query += "AND flag_base_part = 0 ";
    }
    query += get_pool_selector_query();
    query += sort_controller->get_order_by();
    SQLite::Query q(pool.get_db(), query);
    q.bind("$mpn", "%" + MPN_search + "%");
    q.bind("$desc", "%" + desc_search + "%");
    q.bind("$manufacturer", "%" + manufacturer_search + "%");
    q.bind("$entity", entity_uuid);
    q.bind("$entity_all", entity_uuid == UUID());
    bind_tags_query(q, tags);
    bind_pool_selector_query(q);

    if (show_none) {
        row = *(store->append());
        row[list_columns.uuid] = UUID();
        row[list_columns.MPN] = "none";
        row[list_columns.manufacturer] = "none";
        row[list_columns.package] = "none";
    }
    std::list<UUID> uuids;
    try {
        while (q.step()) {
            auto iter = store->append();
            row = *iter;
            UUID uu(q.get<std::string>(0));
            uuids.push_back(uu);
            iter_cache.emplace(uu, iter);
            row[list_columns.uuid] = uu;
            row[list_columns.MPN] = q.get<std::string>(1);
            row[list_columns.manufacturer] = q.get<std::string>(2);
            row[list_columns.package] = q.get<std::string>(3);
            row[list_columns.tags] = q.get<std::string>(4);
            row[list_columns.path] = q.get<std::string>(5);
            row[list_columns.description] = q.get<std::string>(6);
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
    /*
    SQLite::Query q2(pool->db, "EXPLAIN QUERY PLAN " + query.str());
    while (q2.step()) {
        std::cout << q2.get<int>(0) << "|" << q2.get<int>(1) << "|" << q2.get<int>(2) << "|" << q2.get<std::string>(3)
                  << std::endl;
    }
    */

    finish_search();
    if (stock_info_provider)
        stock_info_provider->update_parts(uuids);
}

UUID PoolBrowserPart::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserPart::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}

void PoolBrowserPart::add_copy_name_context_menu_item()
{
    add_context_menu_item("Copy MPN", [this](const UUID &uu) {
        auto part = pool.get_part(uu);
        auto clip = Gtk::Clipboard::get();
        clip->set_text(part->get_MPN());
    });
}

Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> &PoolBrowserPart::get_stock_info_column()
{
    return list_columns.stock_info;
}

} // namespace horizon
