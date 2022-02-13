#include "map_net_tie.hpp"
#include "block/net_tie.hpp"
#include "block/net.hpp"

namespace horizon {

void MapNetTieDialog::ok_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        selection_valid = true;
        selected_uuid = row[list_columns.uuid];
    }
}

void MapNetTieDialog::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        selection_valid = true;
        selected_uuid = row[list_columns.uuid];
        response(Gtk::ResponseType::RESPONSE_OK);
    }
}

MapNetTieDialog::MapNetTieDialog(Gtk::Window *parent, const std::set<class NetTie *> &net_ties)
    : Gtk::Dialog("Map Net tie", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);

    button_ok->signal_clicked().connect(sigc::mem_fun(*this, &MapNetTieDialog::ok_clicked));

    store = Gtk::ListStore::create(list_columns);
    Gtk::TreeModel::Row row;
    for (auto it : net_ties) {
        row = *(store->append());
        row[list_columns.uuid] = it->uuid;
        row[list_columns.net_primary] = it->net_primary->name;
        row[list_columns.net_secondary] = it->net_secondary->name;
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->append_column("Primary net", list_columns.net_primary);
    view->append_column("Secondary net", list_columns.net_secondary);
    view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    view->signal_row_activated().connect(sigc::mem_fun(*this, &MapNetTieDialog::row_activated));


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*view);
    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
