#include "airwire_filter_window.hpp"
#include "block/block.hpp"
#include "util/util.hpp"
#include "airwire_filter.hpp"

namespace horizon {
AirwireFilterWindow *AirwireFilterWindow::create(Gtk::Window *p, const class Block &b, class AirwireFilter &fi)
{
    AirwireFilterWindow *w;
    Glib::RefPtr<Gtk::Builder> x = Gtk::Builder::create();
    x->add_from_resource("/org/horizon-eda/horizon/imp/airwire_filter.ui");
    x->get_widget_derived("window", w, b, fi);
    w->set_transient_for(*p);
    return w;
}

#define GET_WIDGET(name)                                                                                               \
    do {                                                                                                               \
        x->get_widget(#name, name);                                                                                    \
    } while (0)

AirwireFilterWindow::AirwireFilterWindow(BaseObjectType *cobject, const Glib::RefPtr<Gtk::Builder> &x,
                                         const class Block &bl, class AirwireFilter &fi)
    : Gtk::Window(cobject), block(bl), filter(fi)
{
    GET_WIDGET(treeview);
    GET_WIDGET(all_on_button);
    GET_WIDGET(all_off_button);
    GET_WIDGET(placeholder_box);

    store = Gtk::ListStore::create(list_columns);
    treeview->set_model(store);
    {
        auto cr_toggle = Gtk::manage(new Gtk::CellRendererToggle());
        cr_toggle->property_xalign() = 1;
        auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Net"));
        tvc->pack_start(*cr_toggle, false);
        tvc->add_attribute(cr_toggle->property_active(), list_columns.visible);
        cr_toggle->signal_toggled().connect([this](const Glib::ustring &path) {
            auto it = store->get_iter(path);
            if (it) {
                Gtk::TreeModel::Row row = *it;
                row[list_columns.visible] = !row[list_columns.visible];
                filter.set_visible(row[list_columns.net], row[list_columns.visible]);
            }
        });

        auto cr = Gtk::manage(new Gtk::CellRendererText());
        tvc->pack_start(*cr, true);
        tvc->add_attribute(cr->property_text(), list_columns.net_name);
        tvc->set_sort_column(list_columns.net_name);
        treeview->append_column(*tvc);
    }
    treeview->append_column("Airwires", list_columns.n_airwires);
    treeview->get_column(1)->set_sort_column(list_columns.n_airwires);
    store->set_sort_column(list_columns.net_name, Gtk::SORT_ASCENDING);
    store->set_sort_column(list_columns.n_airwires, Gtk::SORT_ASCENDING);
    store->set_sort_func(list_columns.net_name,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.net_name];
                             Glib::ustring b = rb[list_columns.net_name];
                             return strcmp_natural(a, b);
                         });

    filter.signal_changed().connect(sigc::mem_fun(*this, &AirwireFilterWindow::update_from_filter));
    all_on_button->signal_clicked().connect([this] { filter.set_all(true); });
    all_off_button->signal_clicked().connect([this] { filter.set_all(false); });
}

void AirwireFilterWindow::update_from_filter()
{
    store->clear();
    bool all_on = true;
    bool all_off = true;
    for (const auto &it : filter.get_airwires()) {
        if (it.second.visible)
            all_off = false;
        else
            all_on = false;

        Gtk::TreeModel::Row row = *store->append();
        row[list_columns.net] = it.first;
        row[list_columns.net_name] = block.get_net_name(it.first);
        row[list_columns.n_airwires] = it.second.n;
        row[list_columns.visible] = it.second.visible;
    }
    placeholder_box->set_visible(filter.get_airwires().size() == 0);
    all_on_button->set_sensitive(!all_on);
    all_off_button->set_sensitive(!all_off);
}

}; // namespace horizon
