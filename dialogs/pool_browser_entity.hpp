#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool.hpp"
#include "pool_browser_box.hpp"
namespace horizon {


	class PoolBrowserDialogEntity: public Gtk::Dialog {
		public:
		PoolBrowserDialogEntity(Gtk::Window *parent, Pool *ipool);
			UUID selected_uuid;
			bool selection_valid = false;
			//virtual ~MainWindow();
		private :
			Pool *pool;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( entity_name ) ;
						Gtk::TreeModelColumnRecord::add( prefix ) ;
						Gtk::TreeModelColumnRecord::add( n_gates ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( tags ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> entity_name;
					Gtk::TreeModelColumn<Glib::ustring> prefix;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<UUID> uuid;
					Gtk::TreeModelColumn<unsigned int> n_gates;
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
