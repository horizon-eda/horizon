#include "select_included_board.hpp"
#include "board/board.hpp"

namespace horizon {

void SelectIncludedBoardDialog::ok_clicked()
{
    auto it = view->get_selection()->get_selected();
    if (it) {
        Gtk::TreeModel::Row row = *it;
        std::cout << row[list_columns.name] << std::endl;
        valid = true;
        selected_uuid = row[list_columns.uuid];
    }
}

void SelectIncludedBoardDialog::row_activated(const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *column)
{
    auto it = store->get_iter(path);
    if (it) {
        Gtk::TreeModel::Row row = *it;
        std::cout << "act " << row[list_columns.name] << std::endl;
        valid = true;
        selected_uuid = row[list_columns.uuid];
        response(Gtk::ResponseType::RESPONSE_OK);
    }
}

SelectIncludedBoardDialog::SelectIncludedBoardDialog(Gtk::Window *parent, const Board &brd)
    : Gtk::Dialog("Select board", *parent, Gtk::DialogFlags::DIALOG_MODAL | Gtk::DialogFlags::DIALOG_USE_HEADER_BAR)
{
    Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
    add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
    set_default_response(Gtk::ResponseType::RESPONSE_OK);
    set_default_size(300, 300);

    button_ok->signal_clicked().connect(sigc::mem_fun(*this, &SelectIncludedBoardDialog::ok_clicked));

    store = Gtk::ListStore::create(list_columns);
    Gtk::TreeModel::Row row;
    for (const auto &it : brd.included_boards) {
        row = *(store->append());
        row[list_columns.uuid] = it.first;
        row[list_columns.name] = it.second.get_name();
    }

    view = Gtk::manage(new Gtk::TreeView(store));
    view->append_column("Board", list_columns.name);
    view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
    view->signal_row_activated().connect(sigc::mem_fun(*this, &SelectIncludedBoardDialog::row_activated));


    auto sc = Gtk::manage(new Gtk::ScrolledWindow());
    sc->add(*view);
    get_content_area()->pack_start(*sc, true, true, 0);
    get_content_area()->set_border_width(0);

    show_all();
}
} // namespace horizon
