#include "pool_browser.hpp"
#include "pool/pool.hpp"
#include <set>
#include "util/gtk_util.hpp"
#include "util/util.hpp"
#include "cell_renderer_color_box.hpp"
#include "pool/pool_manager.hpp"
#include "tag_entry.hpp"

namespace horizon {
PoolBrowser::PoolBrowser(Pool *p) : Gtk::Box(Gtk::ORIENTATION_VERTICAL, 0), pool(p)
{
    const auto &pools = PoolManager::get().get_pools();
    if (pools.count(pool->get_base_path())) {
        const auto &mpool = pools.at(pool->get_base_path());
        pools_included = mpool.pools_included.size();
        pool_uuid = mpool.uuid;
    }
}

Gtk::Entry *PoolBrowser::create_search_entry(const std::string &label)
{
    auto entry = Gtk::manage(new Gtk::SearchEntry());
    add_search_widget(label, *entry);
    entry->signal_search_changed().connect(sigc::mem_fun(*this, &PoolBrowser::search));
    search_entries.insert(entry);
    return entry;
}

TagEntry *PoolBrowser::create_tag_entry(const std::string &label)
{
    auto entry = Gtk::manage(new TagEntry(*pool, get_type()));
    add_search_widget(label, *entry);
    entry->signal_changed().connect(sigc::mem_fun(*this, &PoolBrowser::search));
    tag_entries.insert(entry);
    return entry;
}

void PoolBrowser::add_search_widget(const std::string &label, Gtk::Widget &w)
{
    auto la = Gtk::manage(new Gtk::Label(label));
    la->get_style_context()->add_class("dim-label");
    la->set_halign(Gtk::ALIGN_END);
    grid->attach(*la, 0, grid_top, 1, 1);
    w.set_hexpand(true);
    grid->attach(w, 1, grid_top, 1, 1);
    grid_top++;
    la->show();
    w.show();
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
        status_box->get_style_context()->add_class("dim-label");
        status_box->property_margin() = 2;
        status_label = Gtk::manage(new Gtk::Label("Status"));
        label_set_tnum(status_label);
        status_label->set_xalign(0);
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

    // dynamic_cast<Gtk::CellRendererText*>(view->get_column_cell_renderer(3))->property_ellipsize()
    // = Pango::ELLIPSIZE_END;
    treeview->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    treeview->signal_row_activated().connect(sigc::mem_fun(*this, &PoolBrowser::row_activated));
    treeview->get_selection()->signal_changed().connect(sigc::mem_fun(*this, &PoolBrowser::selection_changed));
    if (path_column) {
        path_column->set_visible(false);
    }
    treeview->signal_button_press_event().connect_notify([this](GdkEventButton *ev) {
        Gtk::TreeModel::Path path;
        if (ev->button == 3 && context_menu.get_children().size()) {
#if GTK_CHECK_VERSION(3, 22, 0)
            context_menu.popup_at_pointer((GdkEvent *)ev);
#else
            context_menu.popup(ev->button, gtk_get_current_event_time());
#endif
        }
    });
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

void PoolBrowser::go_to(const UUID &uu)
{
    clear_search();
    search_once();
    select_uuid(uu);
    scroll_to_selection();
}

void PoolBrowser::add_context_menu_item(const std::string &label, sigc::slot1<void, UUID> cb)
{
    auto la = Gtk::manage(new Gtk::MenuItem(label));
    la->show();
    context_menu.append(*la);
    la->signal_activate().connect([this, cb] { cb(get_selected()); });
}

PoolBrowser::PoolItemSource PoolBrowser::pool_item_source_from_db(const UUID &uu, bool overridden)
{
    if (overridden && uu == pool_uuid)
        return PoolItemSource::OVERRIDING;
    else if (uu == pool_uuid)
        return PoolItemSource::LOCAL;
    else
        return PoolItemSource::INCLUDED;
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
                        case PoolItemSource::OVERRIDING:
                            tooltip->set_text("Item is from this pool overriding an item from an included pool");
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

        case PoolItemSource::INCLUDED:
            co = Color::new_from_int(252, 175, 62);
            break;

        default:
            co = Color::new_from_int(239, 41, 41);
        }
        Gdk::RGBA va;
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

} // namespace horizon
