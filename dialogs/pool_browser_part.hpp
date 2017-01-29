#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "pool.hpp"
#include "sort_controller.hpp"
namespace horizon {


	class PoolBrowserDialogPart: public Gtk::Dialog {
		public:
		PoolBrowserDialogPart(Gtk::Window *parent, Pool *ipool, const UUID &ientity_uuid);
			UUID selected_uuid;
			ObjectType selected_object_type;
			bool selection_valid = false;
			void set_MPN(const std::string &MPN);
			//virtual ~MainWindow();
		private :
			Pool *pool;
			UUID entity_uuid;

			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add( MPN ) ;
						Gtk::TreeModelColumnRecord::add( manufacturer ) ;
						Gtk::TreeModelColumnRecord::add( package ) ;
						Gtk::TreeModelColumnRecord::add( uuid ) ;
						Gtk::TreeModelColumnRecord::add( tags) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> MPN;
					Gtk::TreeModelColumn<Glib::ustring> manufacturer;
					Gtk::TreeModelColumn<Glib::ustring> package;
					Gtk::TreeModelColumn<Glib::ustring> tags;
					Gtk::TreeModelColumn<UUID> uuid;
			} ;
			ListColumns list_columns;

			class PoolBrowserBoxPart *box;
			Glib::RefPtr<Gtk::ListStore> store;
			std::unique_ptr<SortController> sort_controller;

			void ok_clicked();
			void row_activated(const Gtk::TreeModel::Path& path, Gtk::TreeViewColumn* column);
			bool auto_ok();
			void search();

	};
}
