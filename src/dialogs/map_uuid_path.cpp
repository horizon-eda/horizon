#include "map_uuid_path.hpp"
#include "util/util.hpp"

namespace horizon {

void MapUUIDPathDialog::ok_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        selection_valid = true;
        selected_uuid_path = row[list_columns.uuid_path];
    }
}

void MapUUIDPathDialog::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        selection_valid = true;
        selected_uuid_path = row[list_columns.uuid_path];
        response(Gtk::ResponseType::RESPONSE_OK);
    }
}

MapUUIDPathDialog::MapUUIDPathDialog(Gtk::Window *parent, const std::map<UUIDPath<2>, std::string> &items)
    : Gtk::Dialog("Map Symbol", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);

    button_ok->signal_clicked().connect(sigc::mem_fun(*this, &MapUUIDPathDialog::ok_clicked));

    store = Gtk::ListStore::create(list_columns);
    store->set_sort_func(list_columns.name,
                         [this](const Gtk::TreeModel::iterator &ia, const Gtk::TreeModel::iterator &ib) {
                             Gtk::TreeModel::Row ra = *ia;
                             Gtk::TreeModel::Row rb = *ib;
                             Glib::ustring a = ra[list_columns.name];
                             Glib::ustring b = rb[list_columns.name];
                             return strcmp_natural(a, b);
                         });
    store->set_sort_column(list_columns.name, Gtk::SORT_ASCENDING);
    Gtk::TreeModel::Row row;
    for (const auto &[uu, name] : items) {
        row = *(store->append());
        row[list_columns.uuid_path] = uu;
        row[list_columns.name] = name;
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->append_column("", list_columns.name);
    view->set_headers_visible(false);
    view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    view->signal_row_activated().connect(sigc::mem_fun(*this, &MapUUIDPathDialog::row_activated));


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*view);
    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
