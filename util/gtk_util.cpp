#include "gtk_util.hpp"
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
		sp->signal_changed().connect([sp, &v]{
				v = sp->get_value_as_int();
		});
	}
	void bind_widget(SpinButtonDim *sp, uint64_t &v) {
		sp->set_value(v);
		sp->signal_changed().connect([sp, &v]{
				v = sp->get_value_as_int();
		});
	}

	void bind_widget(Gtk::SpinButton *sp, int &v) {
		sp->set_value(v);
		sp->signal_changed().connect([sp, &v]{
				v = sp->get_value_as_int();
		});
	}
}
