#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
namespace horizon {


	class SelectViaPadstackDialog: public Gtk::Dialog {
		public:
			SelectViaPadstackDialog(Gtk::Window *parent, class ViaPadstackProvider *vpp);
			UUID selected_uuid;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :

			ViaPadstackProvider *via_padstack_provider;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
	};
}
