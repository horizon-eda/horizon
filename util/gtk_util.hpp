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

	template <typename T> void bind_widget(Gtk::ComboBoxText *combo, const std::map<T, std::string> &lut, T &v) {
		for(const auto &it: lut) {
			combo->append(std::to_string(static_cast<int>(it.first)), it.second);
		}
		combo->set_active_id(std::to_string(static_cast<int>(v)));
		combo->signal_changed().connect([combo, &v] {
			v = static_cast<T>(std::stoi(combo->get_active_id()));
		});
	}



	Gtk::Label *grid_attach_label_and_widget(Gtk::Grid *gr, const std::string &label, Gtk::Widget *w, int &top);

	void tree_view_scroll_to_selection(Gtk::TreeView *view);
	void entry_set_warning(Gtk::Entry *e, const std::string &text);

	void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before);
}
