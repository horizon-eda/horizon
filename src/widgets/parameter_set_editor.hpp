#pragma once
#include <gtkmm.h>
#include "parameter/set.hpp"

namespace horizon {
	class ParameterSetEditor: public Gtk::Box {
		friend class ParameterEditor;
		public:
			ParameterSetEditor(ParameterSet *ps, bool populate_init=true);
			void populate();
			void focus_first();
			void set_button_margin_left(int margin);

			typedef sigc::signal<void> type_signal_activate_last;
			type_signal_activate_last signal_activate_last() {return s_signal_activate_last;}

			typedef sigc::signal<void, ParameterID> type_signal_apply_all;
			type_signal_apply_all signal_apply_all() {return s_signal_apply_all;}

		protected:
			virtual Gtk::Widget *create_extra_widget(ParameterID id);

		private:
			Gtk::MenuButton *add_button = nullptr;
			Gtk::ListBox *listbox = nullptr;
			Gtk::Box *popover_box = nullptr;
			ParameterSet *parameter_set;
			Glib::RefPtr<Gtk::SizeGroup> sg_label;
			void update_popover_box();

			type_signal_activate_last s_signal_activate_last;

		protected:
			type_signal_apply_all s_signal_apply_all;
	};
}
