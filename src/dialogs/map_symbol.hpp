#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "util/uuid.hpp"
#include "util/uuid_path.hpp"
namespace horizon {


	class MapSymbolDialog: public Gtk::Dialog {
		public:
			MapSymbolDialog(Gtk::Window *parent, const std::map<UUIDPath<2>, std::string> &gates);
			UUIDPath<2> selected_uuid_path;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( uuid_path ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<UUIDPath<2>> uuid_path;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

	};
}
