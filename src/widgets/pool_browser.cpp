#include "pool_browser.hpp"
#include "pool/ipool.hpp"
#include <set>
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "cell_renderer_color_box.hpp"
#include "pool/pool_manager.hpp"
#include "tag_entry.hpp"
#include "util/sqlite.hpp"
#include "pool_selector.hpp"
#include "util/sort_controller.hpp"

namespace horizon {
PoolBrowser::PoolBrowser(IPool &p, const std::string &prefix)
    : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p), store_prefix(prefix)
{
    const auto &pool_info = pool.get_pool_info();
    pools_included = pool_info.pools_included.size();
    pool_uuid = pool_info.uuid;
}

Gtk::Entry *PoolBrowser::create_search_entry(const std::string &label, Gtk::Widget *extra_widget)
{
    auto entry = Gtk::manage(new Gtk::SearchEntry());
    add_search_widget(label, *entry, extra_widget);
    entry->signal_search_changed().connect(sigc::mem_fun(*this, &PoolBrowser::search));
    search_entries.insert(entry);
    return entry;
}

TagEntry *PoolBrowser::create_tag_entry(const std::string &label, Gtk::Widget *extra_widget)
{
    auto entry = Gtk::manage(new TagEntry(pool, get_type()));
    add_search_widget(label, *entry, extra_widget);
    entry->signal_changed().connect(sigc::mem_fun(*this, &PoolBrowser::search));
    tag_entries.insert(entry);
    return entry;
}

void PoolBrowser::add_search_widget(const std::string &label, Gtk::Widget &w, Gtk::Widget *extra_widget)
{
    auto la = Gtk::manage(new Gtk::Label(label));
    la->get_style_context()->add_class("dim-label");
    la->set_halign(Gtk::ALIGN_END);
    grid->attach(*la, 0, grid_top, 1, 1);
    w.set_hexpand(true);
    if (extra_widget == nullptr) {
        grid->attach(w, 1, grid_top, 1, 1);
        w.show();
    }
    else {
        auto box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 10));
        box->pack_start(w, true, true, 0);
        box->pack_start(*extra_widget, false, false, 0);
        box->show_all();
        grid->attach(*box, 1, grid_top, 1, 1);
    }
    grid_top++;
    la->show();
}

Gtk::Widget *PoolBrowser::create_pool_selector()
{
    if (pools_included) {
        if (pool_selector)
            throw std::runtime_error("can't create pool selector twice");
        pool_selector = Gtk::manage(new PoolSelector(pool));
        pool_selector->signal_changed().connect([this] {
            if (!pools_reloading)
                search();
        });
        return pool_selector;
    }
    else {
        return nullptr;
    }
}

void PoolBrowser::reload_pools()
{
    if (!pool_selector)
        return;
    pools_reloading = true;
    pool_selector->reload();
    pools_reloading = false;
}

std::string PoolBrowser::get_pool_selector_query() const
{
    if (!pool_selector)
        return "";

    if (pool_selector->get_selected_pool() == UUID())
        return "";

    return " AND (" + IPool::type_names.at(get_type()) + ".pool_uuid = $pool_uuid) ";
}

void PoolBrowser::bind_pool_selector_query(SQLite::Query &q) const
{
    if (!pool_selector)
        return;

    if (pool_selector->get_selected_pool() == UUID())
        return;

    q.bind("$pool_uuid", pool_selector->get_selected_pool());
}

void PoolBrowser::install_column_tooltip(Gtk::TreeViewColumn &tvc, const Gtk::TreeModelColumnBase &col)
{
    treeview->set_has_tooltip(true);
    treeview->signal_query_tooltip().connect(
            [this, &col, &tvc](int x, int y, bool keyboard_tooltip, const Glib::RefPtr<Gtk::Tooltip> &tooltip) {
                if (keyboard_tooltip)
                    return false;
                Gtk::TreeModel::Path path;
                Gtk::TreeViewColumn *column;
                int cell_x, cell_y;
                int bx, by;
                treeview->convert_widget_to_bin_window_coords(x, y, bx, by);
                if (!treeview->get_path_at_pos(bx, by, path, column, cell_x, cell_y))
                    return false;
                if (column == &tvc) {
                    Gtk::TreeIter iter(treeview->get_model()->get_iter(path));
                    if (!iter)
                        return false;
                    Glib::ustring val;
                    iter->get_value(col.index(), val);
                    tooltip->set_text(val);
                    treeview->set_tooltip_row(tooltip, path);
                    return true;
                }
                return false;
            });
}

Gtk::TreeViewColumn *PoolBrowser::append_column(const std::string &name, const Gtk::TreeModelColumnBase &column,
                                                Pango::EllipsizeMode ellipsize)

{
    return tree_view_append_column_ellipsis(treeview, name, column, ellipsize);
}
Gtk::TreeViewColumn *PoolBrowser::append_column_with_item_source_cr(const std::string &name,
                                                                    const Gtk::TreeModelColumnBase &column,
                                                                    Pango::EllipsizeMode ellipsize)

{
    auto tvc = Gtk::manage(new Gtk::TreeViewColumn(name));
    auto cr = Gtk::manage(new Gtk::CellRendererText());
    if (pool_uuid && pools_included) {
        auto cr_cb = create_pool_item_source_cr(tvc);
        tvc->pack_start(*cr_cb, false);
    }
    tvc->pack_start(*cr, true);
    tvc->add_attribute(cr->property_text(), column);
    cr->property_ellipsize() = ellipsize;
    treeview->append_column(*tvc);
    return tvc;
}

void PoolBrowser::construct(Gtk::Widget *search_box)
{

    store = create_list_store();

    if (search_box) {
        search_box->show();
        pack_start(*search_box, false, false, 0);
    }
    else {
        grid = Gtk::manage(new Gtk::Grid());
        grid->set_column_spacing(10);
        grid->set_row_spacing(10);

        grid->set_margin_top(20);
        grid->set_margin_start(20);
        grid->set_margin_end(20);
        grid->set_margin_bottom(20);

        grid->show();
        pack_start(*grid, false, false, 0);
    }

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }

    treeview = Gtk::manage(new Gtk::TreeView(store));

    scrolled_window = Gtk::manage(new Gtk::ScrolledWindow());
    scrolled_window->add(*treeview);
    scrolled_window->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    scrolled_window->show();

    {
        auto overlay = Gtk::manage(new Gtk::Overlay);
        overlay->add(*scrolled_window);
        overlay->show_all();
        busy_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_VERTICAL, 4));
        auto img = Gtk::manage(new Gtk::Image);
        img->set_from_icon_name("hourglass-symbolic", Gtk::ICON_SIZE_DIALOG);
        busy_box->pack_start(*img, true, true, 0);
        auto la = Gtk::manage(new Gtk::Label("Database is busy"));
        busy_box->pack_start(*la, true, true, 0);
        auto bu = Gtk::manage(new Gtk::Button("Retry search"));
        bu->signal_clicked().connect([this] { search(); });
        busy_box->pack_start(*bu, true, true, 0);
        busy_box->show_all();
        busy_box->set_valign(Gtk::ALIGN_CENTER);
        busy_box->set_halign(Gtk::ALIGN_CENTER);
        overlay->add_overlay(*busy_box);
        busy_box->hide();
        busy_box->set_no_show_all(true);
        pack_start(*overlay, true, true, 0);
    }

    {
        auto sep = Gtk::manage(new Gtk::Separator(Gtk::ORIENTATION_HORIZONTAL));
        sep->show();
        pack_start(*sep, false, false, 0);
    }

    {
        status_box = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 8));
        status_box->get_style_context()->add_class("pool-browser-status-box");
        status_label = Gtk::manage(new Gtk::Label("Status"));
        label_set_tnum(status_label);
        status_label->set_xalign(0);
        status_label->get_style_context()->add_class("dim-label");
        status_label->property_margin() = 2;
        status_label->show();
        status_box->pack_start(*status_label, false, false, 0);
        pack_start(*status_box, false, false, 0);
        status_box->show_all();
    }


    treeview->show();

    create_columns();
    treeview->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);

    sort_controller = std::make_unique<SortController>(treeview);
    sort_controller->set_simple(true);
    add_sort_controller_columns();
    sort_controller->set_sort(0, SortController::Sort::ASC);
    sort_controller->signal_changed().connect(sigc::mem_fun(*this, &PoolBrowser::search));

    {
        auto la = Gtk::manage(new Gtk::MenuItem("Sort most recently modified first"));
        la->show();
        header_context_menu.append(*la);
        la->signal_activate().connect(sigc::mem_fun(*this, &PoolBrowser::sort_by_mtime));
    }

    treeview->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    treeview->signal_row_activated().connect(sigc::mem_fun(*this, &PoolBrowser::row_activated));
    treeview->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PoolBrowser::selection_changed));
    if (path_column) {
        path_column->set_visible(false);
    }
    treeview->signal_button_press_event().connect_notify([this](GdkEventButton *ev) {
        if (gdk_event_triggers_context_menu((GdkEvent *)ev)) {
            if (treeview->get_bin_window()->gobj() == ev->window) {
                if (context_menu.get_children().size()) {
                    Gtk::TreeModel::Path path;
                    if (treeview->get_path_at_pos(ev->x, ev->y, path)) {
                        if (auto it = store->get_iter(path)) {
                            Gtk::TreeModel::Row row = *it;
                            auto sel = uuid_from_row(row);
                            for (auto &[widget, cb] : menu_item_sensitive_cbs) {
                                widget->set_visible(!sel || cb(sel));
                            }
#if GTK_CHECK_VERSION(3, 22, 0)
                            context_menu.popup_at_pointer((GdkEvent *)ev);
#else
                            context_menu.popup(ev->button, gtk_get_current_event_time());
#endif
                        }
                    }
                }
            }
            else {
#if GTK_CHECK_VERSION(3, 22, 0)
                header_context_menu.popup_at_pointer((GdkEvent *)ev);
#else
                header_context_menu.popup(ev->button, gtk_get_current_event_time());
#endif
            }
        }
    });

    if (store_prefix.size()) {
        state_store.emplace(treeview, store_prefix);
    }
}

void PoolBrowser::set_busy(bool busy)
{
    busy_box->set_visible(busy);
}

void PoolBrowser::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        s_signal_activated.emit();
    }
}

void PoolBrowser::selection_changed()
{
    s_signal_selected.emit();
}

UUID PoolBrowser::get_selected()
{
    auto it = treeview->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        return uuid_from_row(row);
    }
    return UUID();
}

bool PoolBrowser::get_any_selected()
{
    return treeview->get_selection()->count_selected_rows();
}

void PoolBrowser::set_show_none(bool v)
{
    show_none = v;
    if (searched_once)
        search();
}

void PoolBrowser::set_show_path(bool v)
{
    show_path = v;
    if (path_column) {
        path_column->set_visible(v);
    }
    if (searched_once)
        search();
}

void PoolBrowser::scroll_to_selection()
{
    tree_view_scroll_to_selection(treeview);
}

void PoolBrowser::select_uuid(const UUID &uu)
{
    for (const auto &it : store->children()) {
        if (uuid_from_row(*it) == uu) {
            treeview->get_selection()->select(it);
            break;
        }
    }
}

void PoolBrowser::clear_search()
{
    for (auto it : search_entries) {
        it->set_text("");
    }
    for (auto it : tag_entries) {
        it->clear();
    }
}

void PoolBrowser::focus_search()
{
    if (focus_widget)
        focus_widget->grab_focus();
}

void PoolBrowser::go_to(const UUID &uu)
{
    clear_search();
    search_once();
    select_uuid(uu);
    scroll_to_selection();
}

void PoolBrowser::add_context_menu_item(const std::string &label, std::function<void(UUID)> cb,
                                        std::function<bool(UUID)> cb_sensitive)
{
    auto la = Gtk::manage(new Gtk::MenuItem(label));
    la->show();
    context_menu.append(*la);
    la->signal_activate().connect([this, cb] { cb(get_selected()); });
    if (cb_sensitive)
        menu_item_sensitive_cbs.emplace_back(la, cb_sensitive);
}

PoolBrowser::PoolItemSource PoolBrowser::pool_item_source_from_db(const SQLite::Query &q, int idx_uu,
                                                                  int idx_last_uu) const
{
    return pool_item_source_from_db(q.get<std::string>(idx_uu), q.get<std::string>(idx_last_uu));
}


PoolBrowser::PoolItemSource PoolBrowser::pool_item_source_from_db(const UUID &uu, const UUID &last_uu) const
{
    if (uu == pool_uuid && last_uu == UUID())
        return PoolItemSource::LOCAL;
    else if (uu == pool_uuid && last_uu != UUID() && uu == PoolInfo::project_pool_uuid)
        return PoolItemSource::CACHED;
    else if (uu == pool_uuid && last_uu != UUID())
        return PoolItemSource::OVERRIDDEN_LOCAL;
    else if (uu != pool_uuid && last_uu == UUID())
        return PoolItemSource::INCLUDED;
    else if (uu != pool_uuid && last_uu != UUID())
        return PoolItemSource::OVERRIDDEN;
    throw std::logic_error("unhandled case");
}

void PoolBrowser::install_pool_item_source_tooltip()

{
    if (!(pool_uuid && pools_included))
        return;

    treeview->set_has_tooltip(true);
    treeview->signal_query_tooltip().connect([this](int x, int y, bool keyboard_tooltip,
                                                    const Glib::RefPtr<Gtk::Tooltip> &tooltip) {
        if (keyboard_tooltip)
            return false;
        Gtk::TreeModel::Path path;
        Gtk::TreeViewColumn *column;
        int cell_x, cell_y;
        int bx, by;
        treeview->convert_widget_to_bin_window_coords(x, y, bx, by);
        if (!treeview->get_path_at_pos(bx, by, path, column, cell_x, cell_y))
            return false;
        auto cells = column->get_cells();
        for (auto cell : cells) {
            int start, width;
            if (column->get_cell_position(*cell, start, width)) {
                if ((cell_x >= start) && (cell_x < (start + width))) {
                    if (cell == cell_renderer_item_source) {
                        Gtk::TreeIter iter(treeview->get_model()->get_iter(path));
                        if (!iter)
                            return false;
                        PoolItemSource src = pool_item_source_from_row(*iter);
                        switch (src) {
                        case PoolItemSource::LOCAL:
                            tooltip->set_text("Item is from this pool");
                            break;
                        case PoolItemSource::INCLUDED:
                            tooltip->set_text("Item is from included pool");
                            break;
                        case PoolItemSource::OVERRIDDEN_LOCAL:
                            tooltip->set_text("Item is from this pool overriding an item from an included pool");
                            break;
                        case PoolItemSource::CACHED:
                            tooltip->set_text("Item is cached from an included pool");
                            break;
                        case PoolItemSource::OVERRIDDEN:
                            tooltip->set_text("Item is from another pool overriding an item from an included pool");
                            break;
                        }
                        //
                        treeview->set_tooltip_cell(tooltip, &path, column, cell);
                        return true;
                    }
                    break;
                }
            }
        }
        return false;
    });
}

PoolBrowser::PoolItemSource PoolBrowser::pool_item_source_from_row(const Gtk::TreeModel::Row &row)
{
    return PoolItemSource::LOCAL;
}

CellRendererColorBox *PoolBrowser::create_pool_item_source_cr(Gtk::TreeViewColumn *tvc)
{
    auto cr_cb = Gtk::manage(new CellRendererColorBox());
    tvc->set_cell_data_func(*cr_cb, [this](Gtk::CellRenderer *tcr, const Gtk::TreeModel::iterator &it) {
        auto mcr = dynamic_cast<CellRendererColorBox *>(tcr);
        Color co(1, 0, 1);
        switch (pool_item_source_from_row(*it)) {
        case PoolItemSource::LOCAL:
            co = Color::new_from_int(138, 226, 52);
            break;

        case PoolItemSource::CACHED:
            co = Color::new_from_int(0x72, 0x9f, 0xcf);
            break;

        case PoolItemSource::OVERRIDDEN_LOCAL:
            co = Color::new_from_int(0xad, 0x7f, 0xa8);
            break;

        case PoolItemSource::INCLUDED:
            co = Color::new_from_int(252, 175, 62);
            break;

        default:
            co = Color::new_from_int(239, 41, 41);
        }
        Gdk::RGBA va;
        va.set_alpha(1);
        va.set_red(co.r);
        va.set_green(co.g);
        va.set_blue(co.b);
        mcr->property_color() = va;
    });
    cell_renderer_item_source = cr_cb;
    return cr_cb;
}

void PoolBrowser::search_once()
{
    if (!searched_once)
        search();
}

void PoolBrowser::clear_search_once()
{
    searched_once = false;
}

void PoolBrowser::prepare_search()
{
    searched_once = true;
    selected_uuid_before_search = get_selected();
    treeview->unset_model();
    store->clear();
}

void PoolBrowser::finish_search()
{
    treeview->set_model(store);
    select_uuid(selected_uuid_before_search);
    scroll_to_selection();
    auto n_items = store->children().size();
    status_label->set_text(format_digits(n_items, 5) + " " + (n_items == 1 ? "Result" : "Results"));
}

std::string PoolBrowser::get_tags_query(const std::set<std::string> &tags) const
{
    if (tags.size() == 0)
        return "";
    std::string query;
    query += "INNER JOIN (SELECT uuid FROM tags WHERE tags.tag IN (";
    int i = 0;
    for (const auto &it : tags) {
        (void)it;
        query += "$tag" + std::to_string(i) + ", ";
        i++;
    }
    query += "'') AND tags.type = $tag_type "
             "GROUP by tags.uuid HAVING count(*) >= $ntags) as x_tags ON x_tags.uuid = "
             + IPool::type_names.at(get_type()) + ".uuid ";
    return query;
}

void PoolBrowser::bind_tags_query(SQLite::Query &q, const std::set<std::string> &tags) const
{
    int i = 0;
    for (const auto &it : tags) {
        q.bind(("$tag" + std::to_string(i)).c_str(), it);
        i++;
    }
    if (tags.size()) {
        q.bind("$ntags", tags.size());
        q.bind("$tag_type", get_type());
    }
}

PoolBrowser::~PoolBrowser() = default;

void PoolBrowser::sort_by_mtime()
{
    sort_controller->set_sort(mtime_column, SortController::Sort::DESC);
}

} // namespace horizon
