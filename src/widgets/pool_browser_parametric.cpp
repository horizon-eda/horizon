#include "pool_browser_parametric.hpp"
#include "pool/pool.hpp"
#include "pool/part.hpp"
#include "util/util.hpp"

namespace horizon {


static Gtk::TreeViewColumn *create_tvc(const PoolParametric::Column &col,
                                       const Gtk::TreeModelColumn<std::string> &tree_col)
{
    if (col.type == PoolParametric::Column::Type::QUANTITY) {
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn(col.display_name));
        auto cr_val = Gtk::manage(new Gtk::CellRendererText());
        auto cr_unit = Gtk::manage(new Gtk::CellRendererText());
        tvc->set_cell_data_func(*cr_val, [&col, &tree_col](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
            Gtk::TreeModel::Row row = *it;
            auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
            std::string v = row[tree_col];
            auto pos = v.find(' ');
            if (pos == std::string::npos)
                mcr->property_text() = v;
            else
                mcr->property_text() = v.substr(0, pos);
        });
        tvc->set_cell_data_func(*cr_unit,
                                [&col, &tree_col](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
                                    Gtk::TreeModel::Row row = *it;
                                    auto mcr = dynamic_cast<Gtk::CellRendererText *>(tcr);
                                    std::string v = row[tree_col];
                                    auto pos = v.find(' ');
                                    if (pos == std::string::npos)
                                        mcr->property_text() = "";
                                    else
                                        mcr->property_text() = v.substr(pos + 1);
                                });
        cr_val->property_xalign() = 1;
        cr_unit->property_xalign() = 1;
        auto attributes_list = pango_attr_list_new();
        auto attribute_font_features = pango_attr_font_features_new("tnum 1");
        pango_attr_list_insert(attributes_list, attribute_font_features);
        g_object_set(G_OBJECT(cr_val->gobj()), "attributes", attributes_list, NULL);
        pango_attr_list_unref(attributes_list);
        tvc->pack_start(*cr_val, false);
        tvc->pack_start(*cr_unit, false);
        {
            auto cr_empty = Gtk::manage(new Gtk::CellRendererText());
            tvc->pack_start(*cr_empty, true);
        }
        return tvc;
    }
    else {
        return Gtk::manage(new Gtk::TreeViewColumn(col.display_name, tree_col));
    }
}

static double string_to_double(const std::string &s)
{
    double d;
    std::istringstream istr(s);
    istr.imbue(std::locale::classic());
    istr >> d;
    return d;
}

class ParametricFilterBox : public Gtk::Box {
public:
    ParametricFilterBox(PoolParametric *p, const PoolParametric::Column &col)
        : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4), pool(p), column(col)
    {
        store = Gtk::ListStore::create(list_columns);
        if (col.type == PoolParametric::Column::Type::QUANTITY) {
            store->set_sort_func(list_columns.value,
                                 [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                                     Gtk::TreeModel::Row ra = *ia;
                                     Gtk::TreeModel::Row rb = *ib;
                                     std::string a = ra[list_columns.value];
                                     std::string b = rb[list_columns.value];
                                     if (a.size() == 0)
                                         return -1;
                                     else if (b.size() == 0)
                                         return 1;
                                     auto d = string_to_double(a) - string_to_double(b);
                                     return sgn(d);
                                 });
        }
        else {
            store->set_sort_func(list_columns.value,
                                 [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                                     Gtk::TreeModel::Row ra = *ia;
                                     Gtk::TreeModel::Row rb = *ib;
                                     std::string a = ra[list_columns.value];
                                     std::string b = rb[list_columns.value];
                                     if (a.size() == 0)
                                         return -1;
                                     else if (b.size() == 0)
                                         return 1;
                                     return strcmp_natural(a, b);
                                 });
        }
        store->set_sort_column(list_columns.value, Gtk::SORT_ASCENDING);
        view = Gtk::manage(new Gtk::TreeView(store));
        auto tvc = create_tvc(col, list_columns.value_formatted);
        view->append_column(*tvc);
        view->get_column(0)->set_sort_column(list_columns.value);
        view->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
        view->set_rubber_banding(true);
        view->show();
        view->signal_row_activated().connect(
                [this](const Gtk::TreeModel::Path &, Gtk::TreeViewColumn *) { s_signal_activated.emit(); });
        if (col.type == PoolParametric::Column::Type::QUANTITY) {
            view->set_search_equal_func([this](const Glib::RefPtr<Gtk::TreeModel> &model, int c,
                                               const Glib::ustring &needle, const Gtk::TreeModel::iterator &it) {
                auto v = string_to_double(it->get_value(list_columns.value));
                auto needle_f = parse_si(needle);
                if (isnan(needle_f))
                    return true;
                if (std::abs(needle_f) >= 1 && std::abs(needle_f) < 1000) { // ignore si prefix
                    int exp = 0;
                    while (v >= 1e3 && exp <= 12) {
                        v /= 1e3;
                        exp += 3;
                    }
                    if (v > 1e-15) {
                        while (v < 1 && exp >= -12) {
                            v *= 1e3;
                            exp -= 3;
                        }
                    }
                }

                if (std::abs((v - needle_f) / v) < 0.001)
                    return false;
                else
                    return true;
            });
        }
        auto sc = Gtk::manage(new Gtk::ScrolledWindow());
        sc->set_shadow_type(Gtk::SHADOW_IN);
        sc->set_min_content_height(150);
        sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
        sc->add(*view);
        sc->show_all();

        pack_start(*sc, true, true, 0);
    }

    void update(const std::set<std::string> &values)
    {
        set_visible(values.size() > 1);
        std::set<std::string> values_selected;
        for (const auto &path : view->get_selection()->get_selected_rows()) {
            auto it = store->get_iter(path);
            Gtk::TreeModel::Row row = *it;
            values_selected.insert(row[list_columns.value]);
        }
        store->clear();
        for (const auto &value : values) {
            Gtk::TreeIter it;
            gtk_list_store_insert_with_values(store->gobj(), it.gobj(), -1, list_columns.value.index(), value.c_str(),
                                              list_columns.value_formatted.index(), column.format(value).c_str(), -1);
            if (values_selected.count(value))
                view->get_selection()->select(it);
        }
    }

    std::set<std::string> get_values()
    {
        std::set<std::string> r;
        auto sel = view->get_selection();
        for (auto &path : sel->get_selected_rows()) {
            auto it = store->get_iter(path);
            Gtk::TreeModel::Row row = *it;
            r.emplace(row[list_columns.value]);
        }
        return r;
    }

    void reset()
    {
        view->get_selection()->unselect_all();
    }

    typedef sigc::signal<void> type_signal_activated;
    type_signal_activated signal_activated()
    {
        return s_signal_activated;
    }

private:
    PoolParametric *pool;
    const PoolParametric::Column &column;
    Gtk::TreeView *view = nullptr;
    class ListColumns : public Gtk::TreeModelColumnRecord {
    public:
        ListColumns()
        {
            Gtk::TreeModelColumnRecord::add(value);
            Gtk::TreeModelColumnRecord::add(value_formatted);
        }
        Gtk::TreeModelColumn<std::string> value;
        Gtk::TreeModelColumn<std::string> value_formatted;
    };
    ListColumns list_columns;

    Glib::RefPtr<Gtk::ListStore> store;
    type_signal_activated s_signal_activated;
};

PoolBrowserParametric::PoolBrowserParametric(Pool *p, PoolParametric *pp, const std::string &table_name)
    : PoolBrowserStockinfo(p), pool_parametric(pp), table(pp->get_tables().at(table_name)), list_columns(table)
{
    search_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 10));
    search_box->property_margin() = 10;

    auto filters_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));

    auto add_filter_col = [this, &filters_box](auto &col) {
        auto fbox = Gtk::manage(new ParametricFilterBox(pool_parametric, col));
        fbox->signal_activated().connect([this] {
            apply_filters();
            search();
        });
        fbox->show();
        fbox->set_no_show_all(true);
        filters_box->pack_start(*fbox, false, true, 0);
        filter_boxes.emplace(col.name, fbox);
        columns.emplace(col.name, col);
    };

    for (const auto &col : pool_parametric->get_extra_columns()) {
        add_filter_col(col);
    }

    for (const auto &col : table.columns) {
        add_filter_col(col);
    }

    filters_box->show();
    search_box->pack_start(*filters_box, false, false, 0);


    auto hbox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));

    auto search_button = Gtk::manage(new Gtk::Button("Search"));
    search_button->signal_clicked().connect([this] {
        apply_filters();
        search();
    });
    search_button->set_halign(Gtk::ALIGN_START);
    hbox->pack_start(*search_button, false, false, 0);
    search_button->show();

    auto reset_button = Gtk::manage(new Gtk::Button("Reset"));
    reset_button->signal_clicked().connect([this] {
        for (auto &it : filter_boxes) {
            it.second->reset();
        }
        filters_applied.clear();
        search();
    });
    reset_button->set_halign(Gtk::ALIGN_START);
    hbox->pack_start(*reset_button, false, false, 0);
    reset_button->show();

    filters_applied_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 5));
    filters_applied_box->show();
    hbox->pack_start(*filters_applied_box, true, true, 0);


    hbox->show();
    search_box->pack_start(*hbox, false, false, 0);

    construct(search_box);
    scrolled_window->set_policy(Gtk::POLICY_AUTOMATIC, Gtk::POLICY_AUTOMATIC);
    install_pool_item_source_tooltip();
    update_filters();
}

Glib::RefPtr<Gtk::ListStore> PoolBrowserParametric::create_list_store()
{
    return Gtk::ListStore::create(list_columns);
}


void PoolBrowserParametric::create_columns()
{
    {
        auto col = append_column_with_item_source_cr("MPN", list_columns.MPN, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_expand(true);
        col->set_min_width(100);
    }
    {
        auto col = append_column("Manufacturer", list_columns.manufacturer, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(100);
    }
    {
        auto col = append_column("Package", list_columns.package, Pango::ELLIPSIZE_END);
        col->set_resizable(true);
        col->set_min_width(100);
    }
    for (const auto &col : table.columns) {
        auto tvc = create_tvc(col, list_columns.params.at(col.name));
        treeview->append_column(*tvc);
    }
}

void PoolBrowserParametric::add_sort_controller_columns()
{
    sort_controller->add_column(0, "parts.MPN");
    sort_controller->add_column(1, "parts.manufacturer");
    sort_controller->add_column(2, "packages.name");
    for (size_t i = 0; i < table.columns.size(); i++) {
        auto &col = table.columns.at(i);
        sort_controller->add_column(3 + i, "p." + col.name);
    }
}

static std::string get_in(const std::string &prefix, size_t n)
{
    std::string s = "(";
    for (size_t i = 0; i < n - 1; i++) {
        s += "$" + prefix + std::to_string(i) + ", ";
    }
    s += "$" + prefix + std::to_string(n - 1) + ") ";
    return s;
}

static void bind_set(SQLite::Query &q, const std::string &prefix, const std::set<std::string> &values)
{
    size_t i = 0;
    for (const auto &v : values) {
        q.bind(("$" + prefix + std::to_string(i)).c_str(), v);
        i++;
    }
}

void PoolBrowserParametric::apply_filters()
{
    for (auto &it : filter_boxes) {
        auto values = it.second->get_values();
        if (values.size())
            filters_applied[it.first] = values;
        it.second->reset();
    }
}

void PoolBrowserParametric::search()
{
    prepare_search();
    values_remaining.clear();
    iter_cache.clear();

    std::set<std::string> manufacturers;
    if (filters_applied.count("_manufacturer"))
        manufacturers = filters_applied.at("_manufacturer");
    std::set<std::string> packages;
    if (filters_applied.count("_package"))
        packages = filters_applied.at("_package");
    std::string qs;
    if (similar_part_uuid) {
        qs += "WITH RECURSIVE all_bases(uuidx) AS (SELECT $similar_part UNION "
              "SELECT parts.base FROM parts INNER JOIN all_bases ON parts.uuid = uuidx "
              "WHERE parts.base != '00000000-0000-0000-0000-000000000000'), "
              "all_derived(uuidy) AS (SELECT * FROM all_bases UNION "
              "SELECT parts.uuid FROM parts INNER JOIN all_derived ON parts.base = uuidy), "
              "real_bases(uuidz) AS (SELECT DISTINCT parts.base FROM parts INNER JOIN all_derived ON "
              "all_derived.uuidy = parts.base) ";
    }
    qs += "SELECT p.*, parts.MPN, parts.manufacturer, packages.name, parts.filename, parts.pool_uuid, "
          "parts.overridden "
          "FROM "
          + table.name
          + " AS p LEFT JOIN pool.parts USING (uuid) LEFT JOIN pool.packages ON parts.package = packages.uuid ";
    if (similar_part_uuid) {
        qs += "INNER JOIN real_bases ON real_bases.uuidz = parts.base ";
    }
    qs += "WHERE 1 ";
    if (manufacturers.size()) {
        qs += " AND parts.manufacturer IN " + get_in("_manufacturer", manufacturers.size());
    }
    if (packages.size()) {
        qs += " AND packages.name IN " + get_in("_package", packages.size());
    }
    for (const auto &it : filters_applied) {
        if (it.first.at(0) != '_') {
            if (it.second.size()) {
                qs += " AND p." + it.first + " IN " + get_in(it.first, it.second.size());
            }
        }
    }
    qs += sort_controller->get_order_by();
    std::cout << qs << std::endl;
    SQLite::Query q(pool_parametric->db, qs);
    bind_set(q, "_manufacturer", manufacturers);
    bind_set(q, "_package", packages);
    if (similar_part_uuid) {
        q.bind("$similar_part", similar_part_uuid);
    }
    for (const auto &it : filters_applied) {
        if (it.first.at(0) != '_') {
            bind_set(q, it.first, it.second);
        }
    }
    std::list<UUID> uuids;
    try {
        Gtk::TreeModel::Row row;
        while (q.step()) {
            UUID uu(q.get<std::string>(0));
            uuids.push_back(uu);
            auto iter = store->append();
            row = *(iter);
            row[list_columns.uuid] = uu;
            iter_cache.emplace(uu, iter);
            for (size_t i = 0; i < table.columns.size(); i++) {
                auto &col = table.columns.at(i);
                std::string v = q.get<std::string>(1 + i);
                row[list_columns.params.at(col.name)] = col.format(v);
                values_remaining[col.name].emplace(v);
            }
            size_t ofs = table.columns.size() + 1;
            row[list_columns.MPN] = q.get<std::string>(ofs + 0);
            std::string manufacturer = q.get<std::string>(ofs + 1);
            std::string package = q.get<std::string>(ofs + 2);
            row[list_columns.path] = q.get<std::string>(ofs + 3);
            row[list_columns.source] = pool_item_source_from_db(q.get<std::string>(ofs + 4), q.get<int>(ofs + 5));
            row[list_columns.manufacturer] = manufacturer;
            row[list_columns.package] = package;
            values_remaining["_manufacturer"].emplace(manufacturer);
            values_remaining["_package"].emplace(package);
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
    update_filters();
    update_filters_applied();
    if (stock_info_provider)
        stock_info_provider->update_parts(uuids);
}

void PoolBrowserParametric::update_filters()
{
    for (auto &it : filter_boxes) {
        if (values_remaining.count(it.first)) {
            const auto &values = values_remaining.at(it.first);
            it.second->update(values);
        }
        else {
            it.second->set_visible(false);
        }
    }
}


class PoolBrowserParametric::FilterAppliedLabel : public Gtk::Box {
public:
    FilterAppliedLabel(PoolBrowserParametric *p, const PoolParametric::Column &c, const std::set<std::string> &values)
        : Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 2), parent(p), column(c)
    {
        la = Gtk::manage(new Gtk::Label(column.display_name + " (" + std::to_string(values.size()) + ")"));
        pack_start(*la, false, false, 0);
        la->show();
        std::string tooltip;
        for (const auto &it : values) {
            tooltip += column.format(it) + "\n";
        }
        if (tooltip.size())
            tooltip.pop_back();
        la->set_tooltip_text(tooltip);

        bu = Gtk::manage(new Gtk::Button);
        bu->set_image_from_icon_name("window-close-symbolic", Gtk::ICON_SIZE_BUTTON);
        bu->set_relief(Gtk::RELIEF_NONE);
        bu->get_style_context()->add_class("tag-entry-tiny-button");
        bu->get_style_context()->add_class("dim-label");
        bu->show();
        bu->signal_clicked().connect([this] {
            parent->remove_filter(column.name);
            parent->search();
        });
        pack_start(*bu, false, false, 0);
    }

private:
    PoolBrowserParametric *parent;
    const PoolParametric::Column &column;
    Gtk::Label *la = nullptr;
    Gtk::Button *bu = nullptr;
};

void PoolBrowserParametric::update_filters_applied()
{
    {
        auto chs = filters_applied_box->get_children();
        for (auto ch : chs) {
            delete ch;
        }
    }
    for (const auto &it : filters_applied) {
        const auto &col = columns.at(it.first);
        auto l = Gtk::manage(new FilterAppliedLabel(this, col, it.second));
        l->show();
        filters_applied_box->pack_start(*l, false, false, 0);
    }
}

void PoolBrowserParametric::remove_filter(const std::string &col)
{
    filter_boxes.at(col)->reset();
    filters_applied.erase(col);
}

UUID PoolBrowserParametric::uuid_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.uuid];
}
PoolBrowser::PoolItemSource PoolBrowserParametric::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return row[list_columns.source];
}

void PoolBrowserParametric::add_copy_name_context_menu_item()
{
    add_context_menu_item("Copy MPN", [this](const UUID &uu) {
        auto part = pool->get_part(uu);
        auto clip = Gtk::Clipboard::get();
        clip->set_text(part->get_MPN());
    });
}

Gtk::TreeModelColumn<std::shared_ptr<StockInfoRecord>> &PoolBrowserParametric::get_stock_info_column()
{
    return list_columns.stock_info;
}

void PoolBrowserParametric::set_similar_part_uuid(const UUID &uu)
{
    similar_part_uuid = uu;
}

void PoolBrowserParametric::filter_similar(const UUID &uu, float tol)
{
    auto part = pool->get_part(uu);
    if (part->parametric.count("table") == 0)
        return;
    if (part->parametric.at("table") != table.name)
        return;
    for (const auto &col : table.columns) {
        if (part->parametric.count(col.name)) {
            if (col.type == PoolParametric::Column::Type::QUANTITY) {
                if (values_remaining.count(col.name)) {
                    std::string x = part->parametric.at(col.name);
                    auto target = string_to_double(x);
                    auto lo = target * (1 - tol);
                    auto hi = target * (1 + tol);
                    filters_applied[col.name].clear();
                    for (const auto &it : values_remaining.at(col.name)) {
                        auto v = string_to_double(it);
                        if (v >= lo && v <= hi)
                            filters_applied[col.name].insert(it);
                    }
                }
            }
            else {
                filters_applied[col.name] = {part->parametric.at(col.name)};
            }
        }
    }
}

} // namespace horizon
