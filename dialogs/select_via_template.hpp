#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
namespace horizon {


	class SelectViaTemplateDialog: public Gtk::Dialog {
		public:
			SelectViaTemplateDialog(Gtk::Window *parent, class Board *brd);
			UUID selected_uuid;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :

			class Board *board;

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
