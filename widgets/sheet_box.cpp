#include "sheet_box.hpp"
#include <algorithm>
#include <iostream>
#include "core/core_schematic.hpp"

namespace horizon {
	SheetBox::SheetBox(CoreSchematic *c):
		Gtk::Box(Gtk::Orientation::ORIENTATION_VERTICAL, 0),
		core(c)
	{
		auto *la = Gtk::manage(new Gtk::Label());
		la->set_markup("<b>Sheets</b>");
		la->show();
		pack_start(*la, false, false, 0);

		store = Gtk::ListStore::create(list_columns);
		store->set_sort_column(list_columns.index, Gtk::SORT_ASCENDING);
		view = Gtk::manage(new Gtk::TreeView(store));

		view->append_column("", list_columns.index);

		{
			auto cr = Gtk::manage(new Gtk::CellRendererText());
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("Sheet", *cr));
			tvc->add_attribute(*cr, "text", list_columns.name);
			cr->property_editable().set_value(true);
			cr->signal_edited().connect(sigc::mem_fun(this, &SheetBox::name_edited));
			view->append_column(*tvc);
		}
		{
			auto cr = Gtk::manage(new Gtk::CellRendererPixbuf());
			cr->property_icon_name().set_value("dialog-warning-symbolic");
			cr->property_xalign() = 0;
			auto cr_hi = Gtk::manage(new Gtk::CellRendererPixbuf());
			cr_hi->property_icon_name().set_value("display-brightness-symbolic");
			cr_hi->property_xalign() = 0;
			auto tvc = Gtk::manage(new Gtk::TreeViewColumn("", *cr));
			tvc->add_attribute(*cr, "visible", list_columns.has_warnings);
			tvc->pack_start(*cr_hi, false);
			tvc->add_attribute(*cr_hi, "visible", list_columns.has_highlights);
			view->append_column(*tvc);
		}

		view->show();
		auto sc = Gtk::manage(new Gtk::ScrolledWindow());
		sc->set_policy(Gtk::POLICY_NEVER, Gtk::POLICY_AUTOMATIC);
		sc->set_shadow_type(Gtk::SHADOW_IN);
		sc->set_min_content_height(150);
		sc->add(*view);
		sc->show_all();

		view->get_selection()->signal_changed().connect(sigc::mem_fun(this, &SheetBox::selection_changed));
		pack_start(*sc, true, true, 0);


		auto tb = Gtk::manage(new Gtk::Toolbar());
		tb->get_style_context()->add_class("inline-toolbar");
		tb->set_icon_size(Gtk::ICON_SIZE_MENU);
		tb->set_toolbar_style(Gtk::TOOLBAR_ICONS);
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("list-add-symbolic");
			tbo->signal_clicked().connect([this]{s_signal_add_sheet.emit();});
			tb->insert(*tbo, -1);
		}
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("list-remove-symbolic");
			tbo->signal_clicked().connect(sigc::mem_fun(this, &SheetBox::remove_clicked));
			tb->insert(*tbo, -1);
			remove_button = tbo;
		}
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("go-up-symbolic");
			tbo->signal_clicked().connect([this] {sheet_move(-1);});
			tb->insert(*tbo, -1);
			move_up_button = tbo;
		}
		{
			auto tbo = Gtk::manage(new Gtk::ToolButton());
			tbo->set_icon_name("go-down-symbolic");
			tbo->signal_clicked().connect([this] {sheet_move(1);});
			tb->insert(*tbo, -1);
			move_down_button = tbo;
		}
		update();
		selection_changed();
		tb->show_all();
		pack_start(*tb, false, false, 0);

	}

	void SheetBox::sheet_move(int dir) {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			auto sch = core->get_schematic();
			auto &sheets = sch->sheets;
			auto *sheet = &sch->sheets.at(row[list_columns.uuid]);
			if(dir < 0 && sheet->index==1)
				return;
			if(dir > 0 && sheet->index == sch->sheets.size())
				return;
			auto sheet_other = std::find_if(sheets.begin(), sheets.end(), [sheet, dir](const auto x){return x.second.index == sheet->index+dir;});
			assert(sheet_other != sheets.end());
			std::swap(sheet_other->second.index, sheet->index);
			core->commit();
			core->rebuild();
		}
	}

	void SheetBox::remove_clicked() {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			signal_remove_sheet().emit(&core->get_schematic()->sheets.at(row[list_columns.uuid]));
		}
		else {
			signal_remove_sheet().emit(nullptr);
		}
	}

	void SheetBox::selection_changed() {
		auto it = view->get_selection()->get_selected();
		if(it) {
			Gtk::TreeModel::Row row = *it;
			if(core->get_schematic()->sheets.count(row[list_columns.uuid])) {
				auto &sheets = core->get_schematic()->sheets;
				auto sh = &sheets.at(row[list_columns.uuid]);
				signal_select_sheet().emit(sh);
				auto s = sh->symbols.size()==0;
				remove_button->set_sensitive(s);
				move_up_button->set_sensitive(sh->index != 1);
				move_down_button->set_sensitive(sh->index != sheets.size());
			}
		}
	}

	void SheetBox::name_edited(const Glib::ustring& path, const Glib::ustring& new_text) {
		auto it = store->get_iter(path);
		if(it) {
			Gtk::TreeModel::Row row = *it;
			core->get_schematic()->sheets.at(row[list_columns.uuid]).name = new_text;
			core->commit();
			core->rebuild();
		}
	}

	void SheetBox::update_highlights(const UUID &uu, bool v) {
		for(const auto &it: store->children()) {
			Gtk::TreeModel::Row row = *it;
			if(row[list_columns.uuid] == uu) {
				row[list_columns.has_highlights] = v;
				break;
			}
		}
	}

	void SheetBox::update() {
		auto uuid_from_core = core->get_sheet()->uuid;
		auto s = core->get_schematic()->sheets.size()>1;
		remove_button->set_sensitive(s);

		Gtk::TreeModel::Row row;
		store->freeze_notify();
		store->clear();
		for(const auto &it: core->get_schematic()->sheets) {
			row = *(store->append());
			row[list_columns.name] = it.second.name;
			row[list_columns.uuid] = it.first;
			row[list_columns.has_warnings] = it.second.warnings.size()>0;
			row[list_columns.index] = it.second.index;
		}
		store->thaw_notify();


		for(const auto &it: store->children()) {
			row = *it;
			if(row[list_columns.uuid] == uuid_from_core) {
				view->get_selection()->select(it);
				break;
			}
		}
	}

	void SheetBox::select_sheet(const UUID &sheet_uuid) {
		Gtk::TreeModel::Row row;
		for(const auto &it: store->children()) {
			row = *it;
			if(row[list_columns.uuid] == sheet_uuid) {
				view->get_selection()->select(it);
				break;
			}
		}
	}


}
