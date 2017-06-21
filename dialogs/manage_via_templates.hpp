#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
namespace horizon {


	class ManageViaTemplatesDialog: public Gtk::Dialog {
		public:
			ManageViaTemplatesDialog(Gtk::Window *pa, class Board *b, class ViaPadstackProvider *vpp);
			bool valid = false;

		private :
			Board *board = nullptr;
			ViaPadstackProvider *via_padstack_provider = nullptr;
			Gtk::Stack *stack;
			Gtk::ToolButton *delete_button;
			Gtk::Window *parent;
			void add_template();
			void remove_template();
			void update_template_removable();

			void ok_clicked();
	};
}
