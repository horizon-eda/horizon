#include "dialogs.hpp"
#include "map_pin.hpp"
#include "map_symbol.hpp"
#include "map_package.hpp"
#include "pool_browser_symbol.hpp"
#include "pool_browser_entity.hpp"
#include "pool_browser_padstack.hpp"
#include "pool_browser_part.hpp"
#include "component_pin_names.hpp"
#include "select_net.hpp"
#include "ask_net_merge.hpp"
#include "ask_delete_component.hpp"
#include "manage_buses.hpp"
#include "manage_net_classes.hpp"
#include "ask_datum.hpp"
#include "select_via_padstack.hpp"
#include "annotate.hpp"
#include "core/core.hpp"
#include "core/core_schematic.hpp"
#include "core/cores.hpp"
#include "part.hpp"
#include "edit_shape.hpp"

namespace horizon {
	void Dialogs::set_parent(Gtk::Window *w) {
		parent = w;
	}

	std::pair<bool, UUID> Dialogs::map_pin(const std::vector<std::pair<const Pin*, bool>> &pins) {
		MapPinDialog dia(parent, pins);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUIDPath<2>> Dialogs::map_symbol(const std::map<UUIDPath<2>, std::string> &gates) {
		MapSymbolDialog dia(parent, gates);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid_path};
		}
		else {
			return {false, UUIDPath<2>()};
		}
	}

	std::pair<bool, UUID> Dialogs::select_symbol(Core *c, const UUID &unit_uuid) {
		PoolBrowserDialogSymbol dia(parent, c, unit_uuid);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}

	std::pair<bool, UUID> Dialogs::select_padstack(Pool *pool, const UUID &package_uuid) {
		PoolBrowserDialogPadstack dia(parent, pool, package_uuid);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_entity(Pool *pool) {
		PoolBrowserDialogEntity dia(parent, pool);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_net(class Block *block, bool power_only) {
		SelectNetDialog dia(parent, block, "Select net");
		dia.net_selector->set_power_only(power_only);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.valid, dia.net};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_bus(class Block *block) {
		SelectNetDialog dia(parent, block, "Select bus");
		dia.net_selector->set_bus_mode(true);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.valid, dia.net};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_bus_member(class Block *block, const UUID &bus_uuid) {
		SelectNetDialog dia(parent, block, "Select bus member");
		dia.net_selector->set_bus_member_mode(bus_uuid);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.valid, dia.net};
		}
		else {
			return {false, UUID()};
		}
	}

	bool Dialogs::edit_component_pin_names(Component *comp) {
		ComponentPinNamesDialog dia(parent, comp);
		return dia.run() == Gtk::RESPONSE_OK;
	}

	bool Dialogs::edit_shape(Shape *shape) {
		ShapeDialog dia(parent, shape);
		return dia.run() == Gtk::RESPONSE_OK;
	}

	unsigned int Dialogs::ask_net_merge(Net *net, Net *into) {
		AskNetMergeDialog dia(parent, net, into);
		return dia.run();
	}

	bool Dialogs::ask_delete_component(Component *comp) {
		AskDeleteComponentDialog dia(parent, comp);
		return dia.run()==Gtk::RESPONSE_OK;
	}

	bool Dialogs::manage_buses(Block *b) {
		ManageBusesDialog dia(parent, b);
		return dia.run()==Gtk::RESPONSE_OK;
	}

	bool Dialogs::manage_net_classes(Block *b) {
		ManageNetClassesDialog dia(parent, b);
		return dia.run()==Gtk::RESPONSE_OK;
	}

	bool Dialogs::annotate(Schematic *s) {
		AnnotateDialog dia(parent, s);
		return dia.run()==Gtk::RESPONSE_OK;
	}

	std::pair<bool, int64_t> Dialogs::ask_datum(const std::string &label, int64_t def) {
		AskDatumDialog dia(parent, label);
		dia.sp->set_value(def);
		dia.sp->select_region(0, -1);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {true, dia.sp->get_value_as_int()};
		}
		else {
			return {false, 0};
		}
	}
	std::pair<bool, Coordi> Dialogs::ask_datum_coord(const std::string &label, Coordi def) {
		AskDatumDialog dia(parent, label, true);
		dia.sp_x->set_value(def.x);
		dia.sp_y->set_value(def.y);
		dia.sp_x->select_region(0, -1);
		dia.cb_x->set_sensitive(false);
		dia.cb_y->set_sensitive(false);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {true, {dia.sp_x->get_value_as_int(), dia.sp_y->get_value_as_int()}};
		}
		else {
			return {false, Coordi()};
		}
	}

	std::tuple<bool, Coordi, std::pair<bool, bool>> Dialogs::ask_datum_coord2(const std::string &label, Coordi def) {
		AskDatumDialog dia(parent, label, true);
		dia.sp_x->set_value(def.x);
		dia.sp_y->set_value(def.y);
		dia.sp_x->select_region(0, -1);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return std::make_tuple<bool, Coordi, std::pair<bool, bool>>(true, {dia.sp_x->get_value_as_int(), dia.sp_y->get_value_as_int()}, std::make_pair(dia.cb_x->get_active(), dia.cb_y->get_active()));
		}
		else {
			return std::make_tuple<bool, Coordi, std::pair<bool, bool>>(false, Coordi(), {false, false});
		}
	}

	std::pair<bool, UUID> Dialogs::select_part(Pool *pool, const UUID &entity_uuid, const UUID &part_uuid, bool show_none) {
		PoolBrowserDialogPart dia(parent, pool, entity_uuid);
		dia.set_show_none(show_none);
		if(part_uuid) {
			auto part = pool->get_part(part_uuid);
			dia.set_MPN(part->get_MPN());
		}
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}

	std::pair<bool, UUID> Dialogs::map_package(std::set<const Component*> &components) {
		MapPackageDialog dia(parent, components);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}

	std::pair<bool, UUID> Dialogs::select_via_padstack(class ViaPadstackProvider *vpp) {
		SelectViaPadstackDialog dia(parent, vpp);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}

	std::tuple<bool, std::string, int, int64_t, double> Dialogs::ask_dxf_filename(Core *core) {
		Gtk::FileChooserDialog fc(*parent, "Import DXF", Gtk::FILE_CHOOSER_ACTION_OPEN);
		fc.add_button("_Cancel", Gtk::RESPONSE_CANCEL);
		fc.add_button("_Open", Gtk::RESPONSE_ACCEPT);
		auto filter= Gtk::FileFilter::create();
		filter->set_name("DXF drawing");
		filter->add_mime_type("image/vnd.dxf");
		fc.add_filter(filter);

		auto extrabox = Gtk::manage(new Gtk::Box(Gtk::ORIENTATION_HORIZONTAL, 4));
		auto add_extra_label = [extrabox](const std::string &txt) {
			auto la = Gtk::manage(new Gtk::Label(txt));
			extrabox->pack_start(*la, false, false, 0);
		};

		add_extra_label("Layer:");
		auto layer_combo = Gtk::manage(new Gtk::ComboBoxText());
		for(const auto &it: core->get_layer_provider()->get_layers()) {
			layer_combo->insert(0, std::to_string(it.first), it.second.name);
		}
		layer_combo->set_active_id("0");
		extrabox->pack_start(*layer_combo, false, false, 0);
		layer_combo->set_margin_right(4);


		add_extra_label("Line Width:");
		auto width_sp = Gtk::manage(new SpinButtonDim());
		width_sp->set_range(0, 100_mm);
		extrabox->pack_start(*width_sp, false, false, 0);
		width_sp->set_margin_right(4);

		add_extra_label("Scale:");
		auto scale_sp = Gtk::manage(new Gtk::SpinButton());
		scale_sp->set_digits(4);
		scale_sp->set_range(1e-3, 1e3);
		scale_sp->set_value(1);
		scale_sp->set_increments(.1, .1);
		extrabox->pack_start(*scale_sp, false, false, 0);
		scale_sp->set_margin_right(4);


		fc.set_extra_widget(*extrabox);
		extrabox->show_all();


		std::string filename;
		int layer = 0;
		int64_t width=0;
		double scale=1;

		if(fc.run()==Gtk::RESPONSE_ACCEPT) {
			filename = fc.get_filename();
			layer = std::stoi(layer_combo->get_active_id());
			width = width_sp->get_value_as_int();
			scale = scale_sp->get_value();
			return {true, filename, layer, width, scale};
		}
		else {
			return {false, filename, layer, width, scale};
		}
	}

}
