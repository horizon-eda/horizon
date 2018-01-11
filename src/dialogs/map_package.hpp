#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common/common.hpp"
#include "block/component.hpp"
#include "util/uuid.hpp"
namespace horizon {


	class MapPackageDialog: public Gtk::Dialog {
		public:
			MapPackageDialog(Gtk::Window *parent, const std::vector<std::pair<Component*, bool>> &components);
			UUID selected_uuid;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( package) ;
						Gtk::TreeModelColumnRecord::add( uuid) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<Glib::ustring> package;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);

	};
}
