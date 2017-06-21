#include "select_via_template.hpp"
#include "board/board.hpp"
#include <iostream>

namespace horizon {

	void SelectViaTemplateDialog::ok_clicked() {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			std::cout << row[list_columns.name] << std::endl;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
		}
	}

	void SelectViaTemplateDialog::row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			std::cout << "act " << row[list_columns.name] << std::endl;
			selection_valid = true;
			selected_uuid = row[list_columns.uuid];
			response(Gtk::ResponseType::RESPONSE_OK);
		}
	}

	SelectViaTemplateDialog::SelectViaTemplateDialog(Gtk::Window *parent, Board *brd) :
		Gtk::Dialog("Select Via Template", *parent, Gtk::DialogFlags::DIALOG_MODAL|Gtk::DialogFlags::DIALOG_USE_HEADER_BAR),
		board(brd){
		Gtk::Button *button_ok = add_button("OK", Gtk::ResponseType::RESPONSE_OK);
		add_button("Cancel", Gtk::ResponseType::RESPONSE_CANCEL);
		set_default_response(Gtk::ResponseType::RESPONSE_OK);
		set_default_size(300, 300);

		button_ok->signal_clicked().connect(sigc::mem_fun(this, &SelectViaTemplateDialog::ok_clicked));

		store = Gtk::ListStore::create(list_columns);
		Gtk::TreeModel::Row row;
		for(const auto &it: board->via_templates) {
			row = *(store->append());
			row[list_columns.uuid] = it.first;
			row[list_columns.name] = it.second.name;
		}

		view = Gtk::manage(new Gtk::TreeView(store));
		view->append_column("Via Template", list_columns.name);
		view->get_selection()->set_mode(Gtk::SelectionMode::SELECTION_BROWSE);
		view->signal_row_activated().connect(sigc::mem_fun(this, &SelectViaTemplateDialog::row_activated));


		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->add(*view);
		get_content_area()->pack_start(*sc, true, true, 0);
		get_content_area()->set_border_width(0);

		show_all();
	}
}
