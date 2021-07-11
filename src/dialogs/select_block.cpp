#include "select_block.hpp"
#include "blocks/blocks_schematic.hpp"

namespace horizon {

void SelectBlockDialog::ok_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        valid = true;
        selected_uuid = row[list_columns.uuid];
    }
}

void SelectBlockDialog::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        valid = true;
        selected_uuid = row[list_columns.uuid];
        response(Gtk::ResponseType::RESPONSE_OK);
    }
}

SelectBlockDialog::SelectBlockDialog(Gtk::Window *parent, const BlocksSchematic &blocks)
    : Gtk::Dialog("Select block", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);

    button_ok->signal_clicked().connect(sigc::mem_fun(*this, &SelectBlockDialog::ok_clicked));

    store = Gtk::ListStore::create(list_columns);
    Gtk::TreeModel::Row row;
    for (const auto &[uu, it] : blocks.blocks) {
        if (uu != blocks.top_block) {
            row = *(store->append());
            row[list_columns.uuid] = uu;
            row[list_columns.name] = it.block.name;
        }
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->append_column("Block", list_columns.name);
    view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    view->signal_row_activated().connect(sigc::mem_fun(*this, &SelectBlockDialog::row_activated));


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*view);
    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
