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

	void Dialogs::set_core(Core *c) {
		cores = std::make_unique<Cores>(c);
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

	std::pair<bool, UUID> Dialogs::select_symbol(const UUID &unit_uuid) {
		PoolBrowserDialogSymbol dia(parent, cores->r, unit_uuid);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}

	std::pair<bool, UUID> Dialogs::select_padstack(const UUID &package_uuid) {
		PoolBrowserDialogPadstack dia(parent, cores->r->m_pool, package_uuid);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_entity() {
		PoolBrowserDialogEntity dia(parent, cores->r->m_pool);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.selection_valid, dia.selected_uuid};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_net(bool power_only) {
		SelectNetDialog dia(parent, cores->c->get_schematic()->block, "Select net");
		dia.net_selector->set_power_only(power_only);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.valid, dia.net};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_bus() {
		SelectNetDialog dia(parent, cores->c->get_schematic()->block, "Select bus");
		dia.net_selector->set_bus_mode(true);
		auto r = dia.run();
		if(r == Gtk::RESPONSE_OK) {
			return {dia.valid, dia.net};
		}
		else {
			return {false, UUID()};
		}
	}
	std::pair<bool, UUID> Dialogs::select_bus_member(const UUID &bus_uuid) {
		SelectNetDialog dia(parent, cores->c->get_schematic()->block, "Select bus member");
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

	bool Dialogs::manage_buses() {
		ManageBusesDialog dia(parent, cores->c);
		return dia.run()==Gtk::RESPONSE_OK;
	}

	bool Dialogs::annotate() {
		AnnotateDialog dia(parent, cores->c);
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

	std::pair<bool, UUID> Dialogs::select_part(const UUID &entity_uuid, const UUID &part_uuid) {
		PoolBrowserDialogPart dia(parent, cores->r->m_pool, entity_uuid);
		if(part_uuid) {
			auto part = cores->r->m_pool->get_part(part_uuid);
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

}
