#include "component_selector.hpp"
#include <algorithm>
#include <iostream>
#include "block/block.hpp"
#include "util/util.hpp"
#include "util/gtk_util.hpp"

namespace horizon {
ComponentSelector::ComponentSelector(Block *bl) : Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 16), block(bl)
{

    store = Gtk::ListStore::create(list_columns);


    view = Gtk::manage(new Gtk::TreeView(store));
    view->get_selection()->set_mode(Gtk::SELECTION_BROWSE);
    view->append_column("Ref. Desig.", list_columns.name);
    view->get_column(0)->set_sort_column(list_columns.name);
    store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);
    store->set_sort_func(list_columns.name,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.name];
                             Glib::ustring b = rb[list_columns.name];
                             return strcmp_natural(a, b);
                         });
    update();

    view->signal_row_activated().connect(sigc::mem_fun(*this, &ComponentSelector::row_activated));

    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
    sc->add(*view);
    sc->show_all();

    pack_start(*sc, true, true, 0);
}

void ComponentSelector::update()
{

    Gtk::TreeModel::Row row;
    store->freeze_notify();
    store->clear();

    for (const auto &it : block->components) {
        row = *(store->append());
        row[list_columns.name] = it.second.refdes;
        row[list_columns.uuid] = it.second.uuid;
    }

    store->thaw_notify();
    view->grab_focus();
}

UUID ComponentSelector::get_selected_component()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        return row[list_columns.uuid];
    }
    return UUID();
}

void ComponentSelector::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        s_signal_activated.emit(row[list_columns.uuid]);
    }
}

void ComponentSelector::select_component(const UUID &uu)
{
    for (const auto &it : store->children()) {
        Gtk::TreeModel::Row row = *it;
        if (row[list_columns.uuid] == uu) {
            view->get_selection()->select(it);
            break;
        }
    }
    tree_view_scroll_to_selection(view);
}
} // namespace horizon
