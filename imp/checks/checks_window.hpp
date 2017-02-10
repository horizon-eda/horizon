#pragma once
#include <gtkmm.h>
#include <array>
#include <set>
#include "common.hpp"
#include "uuid.hpp"
#include "checks/check.hpp"
namespace horizon {

	class ChecksWindow: public Gtk::Window{
		public:
			ChecksWindow(BaseObjectType* cobject, const Glib::RefPtr<Gtk::Builder>& x);
			static ChecksWindow* create(Gtk::Window *p, class CheckRunner *runner, class CanvasGL *ca);
			typedef sigc::signal<void, Coordi, UUID> type_signal_goto;
			type_signal_goto signal_goto() {return s_signal_goto;}

		private :
			class TreeColumns : public Gtk::TreeModelColumnRecord {
				public:
					TreeColumns() {
						Gtk::TreeModelColumnRecord::add( name ) ;
						Gtk::TreeModelColumnRecord::add( result ) ;
						Gtk::TreeModelColumnRecord::add( comment ) ;
						Gtk::TreeModelColumnRecord::add( has_location ) ;
						Gtk::TreeModelColumnRecord::add( location ) ;
						Gtk::TreeModelColumnRecord::add( sheet ) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<CheckErrorLevel> result;
					Gtk::TreeModelColumn<Glib::ustring> comment;
					Gtk::TreeModelColumn<bool> has_location;
					Gtk::TreeModelColumn<Coordi> location;
					Gtk::TreeModelColumn<UUID> sheet;
			} ;
			TreeColumns tree_columns;

			Glib::RefPtr<Gtk::TreeStore> result_store;


			CheckRunner *check_runner = nullptr;
			CanvasGL *canvas = nullptr;
			Gtk::ListBox *checks_listbox = nullptr;
			Gtk::TreeView *checks_treeview = nullptr;
			Gtk::Button *run_button = nullptr;
			Gtk::Button *load_button = nullptr;
			Gtk::Button *save_button = nullptr;
			std::string last_filename;

			void update_result();
			std::set<CheckID> get_selected_checks();

			type_signal_goto s_signal_goto;
	};
}
