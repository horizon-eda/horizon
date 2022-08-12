#include "part_browser_window.hpp"
#include "util/util.hpp"
#include "widgets/part_preview.hpp"
#include "widgets/pool_browser_part.hpp"
#include "widgets/pool_browser_parametric.hpp"
#include "util/stock_info_provider.hpp"
#include "preferences/preferences_provider.hpp"
#include "util/win32_undef.hpp"
#include "preferences/preferences.hpp"
#include "util/gtk_util.hpp"
#include "pool/pool_cache_status.hpp"

namespace horizon {

static void header_fun(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before)
{
    if (before && !row->get_header()) {
        auto ret = Gtk::manage(new Gtk::Separator);
        row->set_header(*ret);
    }
}

class UUIDBox : public Gtk::Box {
public:
    using Gtk::Box::Box;
    UUID uuid;
};

PartBrowserWindow::PartBrowserWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                     const std::string &pool_path, std::deque<UUID> &favs)
    : Gtk::Window(cobject), pool(pool_path), pool_parametric(pool_path), favorites(favs),
      state_store(this, "part-browser")
{
    x->get_widget("notebook", notebook);
    x->get_widget("menu1", add_search_menu);
    x->get_widget("place_part_button", place_part_button);
    x->get_widget("assign_part_button", assign_part_button);
    x->get_widget("fav_button", fav_button);
    x->get_widget("lb_favorites", lb_favorites);
    x->get_widget("lb_recent", lb_recent);
    x->get_widget("out_of_date_info_bar", out_of_date_info_bar);
    x->get_widget("entity_label", entity_label);
    x->get_widget("entity_info_bar", entity_info_bar);
    info_bar_hide(out_of_date_info_bar);
    info_bar_hide(entity_info_bar);

    lb_favorites->set_header_func(sigc::ptr_fun(header_fun));
    lb_recent->set_header_func(sigc::ptr_fun(header_fun));
    lb_favorites->signal_row_selected().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_favorites_selected));
    lb_favorites->signal_row_activated().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_favorites_activated));
    lb_recent->signal_row_selected().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_favorites_selected));
    lb_recent->signal_row_activated().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_favorites_activated));

    {
        Gtk::Button *pool_cache_button;
        x->get_widget("pool_cache_button", pool_cache_button);
        pool_cache_button->signal_clicked().connect(
                [this] { s_signal_open_pool_cache_window.emit(items_out_of_date); });
    }

    {
        auto la = Gtk::manage(new Gtk::MenuItem("MPN Search"));
        la->signal_activate().connect([this] {
            auto ch = add_search();
            ch->search_once();
        });
        la->show();
        add_search_menu->append(*la);
    }
    for (const auto &it : pool_parametric.get_tables()) {
        auto la = Gtk::manage(new Gtk::MenuItem(it.second.display_name));
        std::string table_name = it.first;
        la->signal_activate().connect([this, table_name] {
            auto ch = add_search_parametric(table_name);
            ch->search_once();
        });
        la->show();
        add_search_menu->append(*la);
    }
    fav_toggled_conn =
            fav_button->signal_toggled().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_fav_toggled));
    place_part_button->signal_clicked().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_place_part));
    assign_part_button->signal_clicked().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_assign_part));

    preview = Gtk::manage(new PartPreview(pool, false, "part_browser"));
    {
        Gtk::Box *box;
        x->get_widget("box", box);
        box->pack_start(*preview, true, true, 0);
    }
    preview->show();

    update_part_current();
    update_favorites();

    auto ch_search = add_search();
    for (const auto &it : pool_parametric.get_tables()) {
        add_search_parametric(it.first);
    }
    notebook->set_current_page(notebook->page_num(*ch_search));
    signal_show().connect(sigc::track_obj([ch_search] { ch_search->search_once(); }, *ch_search));
    signal_show().connect([this] {
        if (needs_reload) {
            needs_reload = false;
            reload();
        }
    });

    notebook->signal_switch_page().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_switch_page));

    entity_info_bar->signal_response().connect([this](auto r) { set_entity({}, ""); });

    {
        Gtk::Paned *paned;
        GET_WIDGET(paned);
        paned_state_store.emplace(paned, "part_browser");
    }
}

void PartBrowserWindow::reload()
{
    pool.clear();
    auto current_page = notebook->get_nth_page(notebook->get_current_page());
    for (auto page : notebook->get_children()) {
        if (auto ch = dynamic_cast<PoolBrowser *>(page)) {
            ch->reload_pools();
            if (current_page == ch)
                ch->search();
            else
                ch->clear_search_once();
        }
    }
}

void PartBrowserWindow::pool_updated(const std::string &p)
{
    for (const auto &[it_path, it_uu] : pool.get_actually_included_pools(true)) {
        if (it_path == p) {
            if (!is_visible()) {
                needs_reload = true;
                return;
            }

            pool_updated_conn.disconnect();
            // debounce since we might receive multiple updated signals in short succession
            pool_updated_conn = Glib::signal_timeout().connect(sigc::track_obj(
                                                                       [this] {
                                                                           reload();
                                                                           return false;
                                                                       },
                                                                       *this),
                                                               500);

            return;
        }
    }
}

void PartBrowserWindow::handle_favorites_selected(Gtk::ListBoxRow *row)
{
    update_part_current();
}

void PartBrowserWindow::handle_favorites_activated(Gtk::ListBoxRow *row)
{
    handle_activate_part();
}

void PartBrowserWindow::handle_place_part()
{
    if (part_current) {
        s_signal_place_part.emit(part_current);
        set_entity(entity_uuid, ""); // clear hint
    }
}

void PartBrowserWindow::handle_assign_part()
{
    if (part_current) {
        s_signal_assign_part.emit(part_current);
        set_entity(entity_uuid, ""); // clear hint
    }
}

void PartBrowserWindow::handle_activate_part()
{
    if (entity_uuid)
        handle_assign_part();
    else
        handle_place_part();
}

void PartBrowserWindow::handle_fav_toggled()
{
    if (part_current) {
        if (fav_button->get_active()) {
            assert(std::count(favorites.begin(), favorites.end(), part_current) == 0);
            favorites.push_front(part_current);
        }
        else {
            assert(std::count(favorites.begin(), favorites.end(), part_current) == 1);
            auto it = std::find(favorites.begin(), favorites.end(), part_current);
            favorites.erase(it);
        }
        update_favorites();
    }
}

void PartBrowserWindow::handle_switch_page(Gtk::Widget *page, guint index)
{
    update_part_current();
    if (notebook->in_destruction())
        return;
    if (auto ch = dynamic_cast<PoolBrowser *>(page))
        ch->search_once();
}

void PartBrowserWindow::placed_part(const UUID &uu)
{
    auto ncount = std::count(recents.begin(), recents.end(), uu);
    assert(ncount < 2);
    if (ncount) {
        auto it = std::find(recents.begin(), recents.end(), uu);
        recents.erase(it);
    }
    recents.push_front(uu);
    update_recents();
}

void PartBrowserWindow::go_to_part(const UUID &uu)
{
    auto page = notebook->get_nth_page(notebook->get_current_page());
    auto br = dynamic_cast<PoolBrowserPart *>(page);
    if (br)
        br->go_to(uu);
    else {
        auto br_new = add_search(uu);
        br_new->search_once();
    }
}

void PartBrowserWindow::update_favorites()
{
    auto children = lb_favorites->get_children();
    for (auto it : children) {
        delete it;
    }

    for (const auto &it : favorites) {
        const Part *part = nullptr;
        try {
            part = pool.get_part(it);
        }
        catch (const std::runtime_error &e) {
            part = nullptr;
        }
        if (part) {
            auto box = Gtk::manage(new UUIDBox(Gtk::ORIENTATION_VERTICAL, 4));
            box->uuid = it;
            auto la_MPN = Gtk::manage(new Gtk::Label());
            la_MPN->set_xalign(0);
            la_MPN->set_markup("<b>" + part->get_MPN() + "</b>");
            box->pack_start(*la_MPN, false, false, 0);

            auto la_mfr = Gtk::manage(new Gtk::Label());
            la_mfr->set_xalign(0);
            la_mfr->set_text(part->get_manufacturer());
            box->pack_start(*la_mfr, false, false, 0);

            box->set_margin_top(4);
            box->set_margin_bottom(4);
            box->set_margin_start(4);
            box->set_margin_end(4);
            lb_favorites->append(*box);
            box->show_all();
        }
    }
}

void PartBrowserWindow::update_recents()
{
    auto children = lb_recent->get_children();
    for (auto it : children) {
        delete it;
    }

    for (const auto &it : recents) {
        auto part = pool.get_part(it);
        if (part) {
            auto box = Gtk::manage(new UUIDBox(Gtk::ORIENTATION_VERTICAL, 4));
            box->uuid = it;
            auto la_MPN = Gtk::manage(new Gtk::Label());
            la_MPN->set_xalign(0);
            la_MPN->set_markup("<b>" + part->get_MPN() + "</b>");
            box->pack_start(*la_MPN, false, false, 0);

            auto la_mfr = Gtk::manage(new Gtk::Label());
            la_mfr->set_xalign(0);
            la_mfr->set_text(part->get_manufacturer());
            box->pack_start(*la_mfr, false, false, 0);

            box->set_margin_top(4);
            box->set_margin_bottom(4);
            box->set_margin_start(4);
            box->set_margin_end(4);
            lb_recent->append(*box);
            box->show_all();
        }
    }
}

void PartBrowserWindow::update_part_current()
{
    if (in_destruction())
        return;
    auto page = notebook->get_nth_page(notebook->get_current_page());
    SelectionProvider *prv = nullptr;
    prv = dynamic_cast<SelectionProvider *>(page);

    if (prv) {
        part_current = prv->get_selected();
    }
    else {
        if (page->get_name() == "fav") {
            auto row = lb_favorites->get_selected_row();
            if (row) {
                part_current = dynamic_cast<UUIDBox *>(row->get_child())->uuid;
            }
            else {
                part_current = UUID();
            }
        }
        else if (page->get_name() == "recent") {
            auto row = lb_recent->get_selected_row();
            if (row) {
                part_current = dynamic_cast<UUIDBox *>(row->get_child())->uuid;
            }
            else {
                part_current = UUID();
            }
        }
        else {
            part_current = UUID();
        }
    }
    auto ncount = std::count(favorites.begin(), favorites.end(), part_current);
    assert(ncount < 2);
    fav_toggled_conn.block();
    fav_button->set_active(ncount > 0);
    fav_toggled_conn.unblock();

    place_part_button->set_sensitive(part_current);
    assign_part_button->set_sensitive(part_current && can_assign);
    fav_button->set_sensitive(part_current);
    if (part_current) {
        auto part = pool.get_part(part_current);
        preview->load(part);
    }
    else {
        preview->load(nullptr);
    }
    update_out_of_date_info_bar();
}

void PartBrowserWindow::set_pool_cache_status(const PoolCacheStatus &st)
{
    pool_cache_status = &st;
    update_out_of_date_info_bar();
}

void PartBrowserWindow::update_out_of_date_info_bar()
{
    info_bar_hide(out_of_date_info_bar);
    items_out_of_date.clear();
    if (!part_current || !pool_cache_status) {
        return;
    }
    SQLite::Query q(pool.db,
                    "WITH RECURSIVE deps(typex, uuidx) AS "
                    "( SELECT 'part', ? UNION "
                    "SELECT dep_type, dep_uuid FROM dependencies, deps "
                    "WHERE dependencies.type = deps.typex AND dependencies.uuid = deps.uuidx) "
                    "SELECT * FROM deps UNION SELECT 'symbol', symbols.uuid FROM symbols "
                    "INNER JOIN deps ON (symbols.unit = uuidx AND typex = 'unit')");
    q.bind(1, part_current);
    while (q.step()) {
        auto type = q.get<ObjectType>(0);
        UUID uu(q.get<std::string>(1));

        auto r = std::find_if(pool_cache_status->items.begin(), pool_cache_status->items.end(),
                              [type, uu](const PoolCacheStatus::Item &it) { return it.type == type && it.uuid == uu; });
        if (r != pool_cache_status->items.end()) {
            if (r->state == PoolCacheStatus::Item::State::OUT_OF_DATE) {
                items_out_of_date.emplace(type, uu);
            }
        }
    }
    if (items_out_of_date.size()) {
        info_bar_show(out_of_date_info_bar);
    }
}

void PartBrowserWindow::set_can_assign(bool v)
{
    can_assign = v;
    assign_part_button->set_sensitive(part_current && can_assign);
}

PoolBrowserPart *PartBrowserWindow::add_search(const UUID &part)
{
    auto ch = Gtk::manage(new PoolBrowserPart(pool, UUID(), "part_browser"));
    ch->set_include_base_parts(false);
    ch->set_entity_uuid(entity_uuid);
    if (auto prv = StockInfoProvider::create(pool.get_base_path())) {
        ch->add_stock_info_provider(std::move(prv));
    }
    ch->get_style_context()->add_class("background");
    auto tab_label = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    auto la = Gtk::manage(new Gtk::Label("MPN Search"));
    la->set_xalign(1);
    auto close_button = Gtk::manage(new Gtk::Button());
    close_button->set_relief(Gtk::RELIEF_NONE);
    close_button->set_image_from_icon_name("window-close-symbolic");
    close_button->signal_clicked().connect([ch] { delete ch; });
    tab_label->pack_start(*close_button, false, false, 0);
    tab_label->pack_start(*la, true, true, 0);
    ch->show_all();
    tab_label->show_all();
    auto index = notebook->append_page(*ch, *tab_label);
    notebook->set_current_page(index);

    search_views.insert(ch);
    ch->signal_selected().connect(sigc::mem_fun(*this, &PartBrowserWindow::update_part_current));
    ch->signal_activated().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_activate_part));
    if (part)
        ch->go_to(part);
    ch->focus_search();
    return ch;
}

PoolBrowserParametric *PartBrowserWindow::add_search_parametric(const std::string &table_name)
{
    auto ch = Gtk::manage(new PoolBrowserParametric(pool, pool_parametric, table_name, "part_browser"));
    if (auto prv = StockInfoProvider::create(pool.get_base_path())) {
        ch->add_stock_info_provider(std::move(prv));
    }
    ch->add_copy_name_context_menu_item();
    ch->set_entity_uuid(entity_uuid);
    ch->get_style_context()->add_class("background");
    auto tab_label = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 0));
    auto la = Gtk::manage(new Gtk::Label(pool_parametric.get_tables().at(table_name).display_name));
    la->set_xalign(1);
    auto close_button = Gtk::manage(new Gtk::Button());
    close_button->set_relief(Gtk::RELIEF_NONE);
    close_button->set_image_from_icon_name("window-close-symbolic");
    close_button->signal_clicked().connect([ch] { delete ch; });
    tab_label->pack_start(*close_button, false, false, 0);
    tab_label->pack_start(*la, true, true, 0);
    ch->show_all();
    tab_label->show_all();
    auto index = notebook->append_page(*ch, *tab_label);
    notebook->set_current_page(index);

    search_views.insert(ch);
    ch->signal_selected().connect(sigc::mem_fun(*this, &PartBrowserWindow::update_part_current));
    ch->signal_activated().connect(sigc::mem_fun(*this, &PartBrowserWindow::handle_activate_part));

    return ch;
}

void PartBrowserWindow::focus_search()
{
    auto page = notebook->get_nth_page(notebook->get_current_page());
    if (auto br = dynamic_cast<PoolBrowserPart *>(page)) {
        br->focus_search();
    }
}

void PartBrowserWindow::set_entity(const UUID &uu, const std::string &hint)
{
    entity_uuid = uu;
    if (entity_uuid) {
        auto entity = pool.get_entity(entity_uuid);
        std::string label = "Only showing parts for Entity “" + entity->name + "”";
        if (hint.size())
            label += ", " + hint;
        entity_label->set_text(label);
        info_bar_show(entity_info_bar);
        place_part_button->get_style_context()->remove_class("suggested-action");
        assign_part_button->get_style_context()->add_class("suggested-action");
    }
    else {
        info_bar_hide(entity_info_bar);
        assign_part_button->get_style_context()->remove_class("suggested-action");
        place_part_button->get_style_context()->add_class("suggested-action");
    }
    for (auto page : notebook->get_children()) {
        if (auto ch = dynamic_cast<PoolBrowserPart *>(page)) {
            ch->set_entity_uuid(entity_uuid);
        }
        else if (auto ch_para = dynamic_cast<PoolBrowserParametric *>(page)) {
            ch_para->set_entity_uuid(entity_uuid);
        }
    }
    if (auto ch = dynamic_cast<PoolBrowser *>(notebook->get_nth_page(notebook->get_current_page()))) {
        ch->search_once();
    }
}

PartBrowserWindow *PartBrowserWindow::create(Gtk::Window *p, const std::string &pool_path, std::deque<UUID> &favs)
{
    PartBrowserWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource(
            "/org/horizon-eda/horizon/pool-prj-mgr/prj-mgr/part_browser/"
            "part_browser.ui");
    x->get_widget_derived("window", w, pool_path, favs);

    return w;
}
} // namespace horizon
