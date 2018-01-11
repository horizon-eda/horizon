#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "core/core_package.hpp"
#include <experimental/optional>
namespace horizon {

	class FootprintGeneratorWindow: public Gtk::Window{
		public:
			FootprintGeneratorWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static FootprintGeneratorWindow* create(Gtk::Window *p, CorePackage *c);
			typedef sigc::signal<void> type_signal_generated;
			type_signal_generated signal_generated() {return s_signal_generated;}

		private :
			Gtk::Stack *stack;
			CorePackage *core;
			Gtk::Button *generate_button;
			void update_can_generate();
			type_signal_generated s_signal_generated;
	};
}
