#pragma once
#include <gtkmm.h>
#include <stdint.h>
namespace horizon {
	void bind_widget(class SpinButtonDim *sp, int64_t &v);
	void bind_widget(class SpinButtonDim *sp, uint64_t &v);

	void bind_widget(Gtk::Switch *sw, bool &v);
	void bind_widget(Gtk::CheckButton *sw, bool &v);
	void bind_widget(Gtk::SpinButton *sp, int &v);
	void bind_widget(Gtk::Scale *sc, float &v);
	void bind_widget(Gtk::Entry *en, std::string &v);
	template <typename T> void bind_widget(std::map<T, Gtk::RadioButton*> &widgets, T &v) {
		widgets[v]->set_active(true);
		for(auto &it: widgets) {
			T key = it.first;
			Gtk::RadioButton *w = it.second;
			it.second->signal_toggled().connect([key, w, &v] {
				if(w->get_active())
					v = key;
			});
		}
	}

	Gtk::Label *grid_attach_label_and_widget(Gtk::Grid *gr, const std::string &label, Gtk::Widget *w, int &top);

	void tree_view_scroll_to_selection(Gtk::TreeView *view);
}
