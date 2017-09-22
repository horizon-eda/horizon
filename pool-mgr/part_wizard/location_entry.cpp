#include "location_entry.hpp"

namespace horizon {

	void LocationEntry::set_filename(const std::string &s) {
		entry->set_text(s);
	}

	std::string LocationEntry::get_filename(){
		return entry->get_text();
	}

	LocationEntry::LocationEntry(): Gtk::Box() {
		get_style_context()->add_class("linked");
		entry = Gtk::manage(new Gtk::Entry());
		pack_start(*entry, true, true, 0);
		entry->show();

		auto button = Gtk::manage(new Gtk::Button("Browse..."));
		pack_start(*button, false, false, 0);
		button->signal_clicked().connect(sigc::mem_fun(this, &LocationEntry::handle_button));
		button->show();
	}

	void LocationEntry::handle_button() {
		auto top = dynamic_cast<Gtk::Window*>(get_ancestor(GTK_TYPE_WINDOW));
		Gtk::FileChooserDialog fc(*top, "Save", Gtk::FILE_CHOOSER_ACTION_SAVE);
		fc.set_do_overwrite_confirmation(true);
		fc.set_filename(entry->get_text());
		fc.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
		fc.add_button("Set", Gtk::RESPONSE_ACCEPT);
		if(fc.run()==Gtk::RESPONSE_ACCEPT) {
			entry->set_text(fc.get_filename());
		}
	}



}
