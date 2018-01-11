#pragma once
#include <gtkmm.h>
#include "util/uuid.hpp"

namespace horizon {
	class SheetBox: public Gtk::Box {
		public:
			SheetBox(class CoreSchematic *c);

			void update();
			void update_highlights(const UUID &uu, bool has_highlights);
			void select_sheet(const UUID &sheet_uuid);
			typedef sigc::signal<void, class Sheet*> type_signal_select_sheet;
			type_signal_select_sheet signal_select_sheet() {return s_signal_select_sheet;}
			typedef sigc::signal<void> type_signal_add_sheet;
			type_signal_add_sheet signal_add_sheet() {return s_signal_add_sheet;}
			typedef sigc::signal<void, Sheet*> type_signal_remove_sheet;
			type_signal_remove_sheet signal_remove_sheet() {return s_signal_remove_sheet;}

		private:
			class ListColumns : public Gtk::TreeModelColumnRecord {
				public:
					ListColumns() {
						Gtk::TreeModelColumnRecord::add(name) ;
						Gtk::TreeModelColumnRecord::add(uuid) ;
						Gtk::TreeModelColumnRecord::add(index) ;
						Gtk::TreeModelColumnRecord::add(has_warnings) ;
						Gtk::TreeModelColumnRecord::add(has_highlights) ;
					}
					Gtk::TreeModelColumn<Glib::ustring> name;
					Gtk::TreeModelColumn<UUID> uuid;
					Gtk::TreeModelColumn<unsigned int> index;
					Gtk::TreeModelColumn<bool> has_warnings;
					Gtk::TreeModelColumn<bool> has_highlights;
			} ;
			ListColumns list_columns;

			CoreSchematic *core;
			Gtk::ToolButton *remove_button = nullptr;
			Gtk::ToolButton *move_up_button = nullptr;
			Gtk::ToolButton *move_down_button = nullptr;

			Gtk::TreeView *view;
			Glib::RefPtr<Gtk::ListStore> store;

			type_signal_select_sheet s_signal_select_sheet;
			type_signal_add_sheet s_signal_add_sheet;
			type_signal_remove_sheet s_signal_remove_sheet;
			void selection_changed(void);
			void remove_clicked(void);
			void name_edited(const Glib::ustring& path, const Glib::ustring& new_text);
			void sheet_move(int dir);
	};


}
