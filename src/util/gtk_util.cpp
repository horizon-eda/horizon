#include "util/gtk_util.hpp"
#include "widgets/spin_button_dim.hpp"

namespace horizon {
	void bind_widget(Gtk::Switch *sw, bool &v) {
		sw->set_active(v);
		sw->property_active().signal_changed().connect([sw, &v]{
				v= sw->get_active();
		});
	}
	void bind_widget(Gtk::CheckButton *cb, bool &v) {
		cb->set_active(v);
		cb->signal_toggled().connect([cb, &v]{
				v= cb->get_active();
		});
	}
	void bind_widget(SpinButtonDim *sp, int64_t &v) {
		sp->set_value(v);
		sp->signal_value_changed().connect([sp, &v]{
				v = sp->get_value_as_int();
		});
	}
	void bind_widget(SpinButtonDim *sp, uint64_t &v) {
		sp->set_value(v);
		sp->signal_value_changed().connect([sp, &v]{
				v = sp->get_value_as_int();
		});
	}

	void bind_widget(Gtk::SpinButton *sp, int &v) {
		sp->set_value(v);
		sp->signal_value_changed().connect([sp, &v]{
				v = sp->get_value_as_int();
		});
	}

	void bind_widget(Gtk::Scale *sc, float &v) {
		sc->set_value(v);
		sc->signal_value_changed().connect([sc, &v]{
				v = sc->get_value();
		});
	}

	void bind_widget(Gtk::Entry *en, std::string &v) {
		en->set_text(v);
		en->signal_changed().connect([en, &v]{
				v = en->get_text();
		});
	}

	Gtk::Label *grid_attach_label_and_widget(Gtk::Grid *gr, const std::string &label, Gtk::Widget *w, int &top) {
		auto la = Gtk::manage(new Gtk::Label(label));
		la->get_style_context()->add_class("dim-label");
		la->set_halign(Gtk::ALIGN_END);
		la->show();
		gr->attach(*la, 0, top, 1, 1);
		w->show();
		gr->attach(*w, 1, top, 1, 1);
		top++;
		return la;
	}

	void tree_view_scroll_to_selection(Gtk::TreeView *view) {
		auto rows = view->get_selection()->get_selected_rows();
		if(rows.size()) {
			view->scroll_to_cell(rows[0], *view->get_column(0));
		}
	}

	void entry_set_warning(Gtk::Entry *e, const std::string &text) {
		if(text.size()) {
			e->set_icon_from_icon_name("dialog-warning-symbolic", Gtk::ENTRY_ICON_SECONDARY);
			e->set_icon_tooltip_text(text, Gtk::ENTRY_ICON_SECONDARY);
		}
		else {
			e->unset_icon(Gtk::ENTRY_ICON_SECONDARY);
		}
	}

	void header_func_separator(Gtk::ListBoxRow *row, Gtk::ListBoxRow *before) {
		if (before && !row->get_header())	{
			auto ret = Gtk::manage(new Gtk::Separator);
			row->set_header(*ret);
		}
	}
}
