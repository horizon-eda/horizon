#pragma once
#include <gtkmm.h>
#include <stdint.h>
namespace horizon {
	void bind_widget(class SpinButtonDim *sp, int64_t &v);
	void bind_widget(class SpinButtonDim *sp, uint64_t &v);

	void bind_widget(Gtk::Switch *sw, bool &v);
	void bind_widget(Gtk::CheckButton *sw, bool &v);
	void bind_widget(Gtk::SpinButton *sp, int &v);
}
