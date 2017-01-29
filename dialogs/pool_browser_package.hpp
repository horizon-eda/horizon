#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool_browser_box.hpp"

namespace horizon {


	class PoolBrowserDialogPackage: public Gtk::Dialog {
		public:
		PoolBrowserDialogPackage(Gtk::Window *parent, class Pool *ipool);
			UUID selected_uuid;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			Pool *pool;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( package_name ) ;
						Gtk::TreeModelColumnRecord::add( n_pads ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( tags) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> package_name;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<UUID> uuid;
					Gtk::TreeModelColumn<unsigned int> n_pads;
			} ;
			ListColumns list_columns;

			PoolBrowserBox *box;
			Glib::RefPtr<Gtk::ListStore> store;


			void search();
			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
			bool auto_ok();

	};
}
