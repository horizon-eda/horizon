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

		GtkFileChooserNative *native = gtk_file_chooser_native_new ("Save",
			top->gobj(),
			GTK_FILE_CHOOSER_ACTION_SAVE,
			"Set",
			"_Cancel");
		auto chooser = Glib::wrap(GTK_FILE_CHOOSER(native));
		chooser->set_do_overwrite_confirmation(true);
		chooser->set_filename(entry->get_text());

		if(gtk_native_dialog_run (GTK_NATIVE_DIALOG (native))==GTK_RESPONSE_ACCEPT) {
			entry->set_text(chooser->get_filename());
		}
	}



}
