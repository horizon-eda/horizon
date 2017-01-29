#pragma once
#include "pool.hpp"
#include <gtkmm.h>
#include "part.hpp"

class PartEditor {
	public:
		PartEditor(const std::string &fname, bool create=false);

		void run();

	private:
		std::string filename;
		horizon::Pool pool;
		horizon::Part part;
		class PartEditorWindow *win = nullptr;
		class EntityButton *entity_button;
		class PackageButton *package_button;
		class PartButton *part_button;
		void update_mapped();
		void update_treeview();
		void update_tags_inherit();
		void save();


		class PinListColumns : public Gtk::TreeModelColumnRecord {
			public:
				PinListColumns() {
					Gtk::TreeModelColumnRecord::add( gate_name ) ;
					Gtk::TreeModelColumnRecord::add( gate_uuid ) ;
					Gtk::TreeModelColumnRecord::add( pin_name ) ;
					Gtk::TreeModelColumnRecord::add( pin_uuid ) ;
					Gtk::TreeModelColumnRecord::add( mapped ) ;
				}
				Gtk::TreeModelColumn<Glib::ustring> gate_name;
				Gtk::TreeModelColumn<Glib::ustring> pin_name;
				Gtk::TreeModelColumn<horizon::UUID> gate_uuid;
				Gtk::TreeModelColumn<horizon::UUID> pin_uuid;
				Gtk::TreeModelColumn<bool> mapped;
		} ;
		PinListColumns pin_list_columns;

		Glib::RefPtr<Gtk::ListStore> pin_store;

		class PadListColumns : public Gtk::TreeModelColumnRecord {
			public:
				PadListColumns() {
					Gtk::TreeModelColumnRecord::add( pad_name ) ;
					Gtk::TreeModelColumnRecord::add( pad_uuid ) ;
					Gtk::TreeModelColumnRecord::add( gate_name ) ;
					Gtk::TreeModelColumnRecord::add( gate_uuid ) ;
					Gtk::TreeModelColumnRecord::add( pin_name ) ;
					Gtk::TreeModelColumnRecord::add( pin_uuid ) ;
				}
				Gtk::TreeModelColumn<Glib::ustring> pad_name;
				Gtk::TreeModelColumn<horizon::UUID> pad_uuid;
				Gtk::TreeModelColumn<Glib::ustring> gate_name;
				Gtk::TreeModelColumn<Glib::ustring> pin_name;
				Gtk::TreeModelColumn<horizon::UUID> gate_uuid;
				Gtk::TreeModelColumn<horizon::UUID> pin_uuid;
		} ;
		PadListColumns pad_list_columns;

		Glib::RefPtr<Gtk::ListStore> pad_store;
};
