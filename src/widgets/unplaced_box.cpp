#include "unplaced_box.hpp"
#include <algorithm>
#include <iostream>
#include "util/util.hpp"

namespace horizon {
UnplacedBox::UnplacedBox(const std::string &title) : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0)
{
    auto *la = Gtk::manage(new Gtk::Label());
    la->set_markup("<b>Unplaced</b>");
    la->set_margin_bottom(2);
    la->show();
    pack_start(*la, false, false, 0);


    store = Gtk::ListStore::create(list_columns);
    store->set_sort_column(list_columns.text, Gtk::SORT_ASCENDING);
    store->set_sort_func(list_columns.text,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.text];
                             Glib::ustring b = rb[list_columns.text];
                             return strcmp_natural(a, b);
                         });
    view = Gtk::manage(new Gtk::TreeView(store));
    view->get_selection()->set_mode(Gtk::SELECTION_MULTIPLE);
    view->set_rubber_banding(true);
    view->append_column(title, list_columns.text);
    view->get_column(0)->set_sort_column(list_columns.text);
    view->get_selection()->signal_changed().connect([this] {
        button_place->set_sensitive(view->get_selection()->count_selected_rows());
        auto paths = view->get_selection()->get_selected_rows();
        std::vector<UUIDPath<2>> uuids;
        for (const auto &path : paths) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                uuids.emplace_back(row[list_columns.uuid]);
            }
        }
        if (uuids.size()) {
            s_signal_selected.emit(uuids);
        }
    });
    view->show();

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->set_shadow_type(Gtk::SHADOW_IN);
    sc->set_min_content_height(150);
    sc->set_propagate_natural_height(true);
    sc->add(*view);
    sc->show_all();

    view->signal_row_activated().connect(sigc::mem_fun(*this, &UnplacedBox::row_activated));
    pack_start(*sc, true, true, 0);


    auto tb = Gtk::manage(new Gtk::Toolbar());
    tb->get_style_context()->add_class("inline-toolbar");
    tb->set_icon_size(Gtk::ICON_SIZE_MENU);
    tb->set_toolbar_style(Gtk::TOOLBAR_ICONS);
    {
        auto tbo = Gtk::manage(new Gtk::ToolButton("Place"));
        tbo->signal_clicked().connect([this] {
            auto paths = view->get_selection()->get_selected_rows();
            std::vector<UUIDPath<2>> uuids;
            for (const auto &path : paths) {
                auto it = store->get_iter(path);
                if (it) {
                    Gtk::TreeModel::Row row = *it;
                    uuids.emplace_back(row[list_columns.uuid]);
                }
            }
            if (uuids.size()) {
                s_signal_place.emit(uuids);
            }
        });
        button_place = tbo;
        tb->insert(*tbo, -1);
    }
    tb->show_all();
    button_place->set_sensitive(false);
    pack_start(*tb, false, false, 0);
}

void UnplacedBox::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        s_signal_place.emit({row[list_columns.uuid]});
    }
}

void UnplacedBox::update(const std::map<UUIDPath<2>, std::string> &items)
{
    s_signal_selected.block();
    set_visible(items.size());
    std::set<UUIDPath<2>> items_available;
    {
        auto ch = store->children();
        for (auto it = ch.begin(); it != ch.end();) {
            Gtk::TreeModel::Row row = *it;
            if (items.count(row[list_columns.uuid]) == 0) {
                store->erase(it++);
            }
            else {
                it++;
                row[list_columns.text] = items.at(row[list_columns.uuid]);
                items_available.emplace(row[list_columns.uuid]);
            }
        }
    }
    for (const auto &[uu, name] : items) {
        if (items_available.count(uu) == 0) {
            Gtk::TreeModel::Row row = *(store->append());
            row[list_columns.text] = name;
            row[list_columns.uuid] = uu;
        }
    }
    s_signal_selected.unblock();
}

void UnplacedBox::set_title(const std::string &title)
{
    view->get_column(0)->set_title(title);
}

} // namespace horizon
