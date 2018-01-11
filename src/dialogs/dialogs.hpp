#pragma once
#include <memory>
#include "util/uuid.hpp"
#include "common/common.hpp"
#include "util/uuid_path.hpp"
#include "block/component.hpp"
#include "parameter/set.hpp"
#include <map>

namespace Gtk {
	class Window;
}

namespace horizon {
	class Dialogs {
		public:
			Dialogs() {};
			void set_parent(Gtk::Window *w);

			std::pair<bool, UUID> map_pin(const std::vector<std::pair<const Pin*, bool>> &pins);
			std::pair<bool, UUIDPath<2>> map_symbol(const std::map<UUIDPath<2>, std::string> &gates);
			std::pair<bool, UUID> map_package(const std::vector<std::pair<Component*, bool>> &components);
			std::pair<bool, UUID> select_symbol(class Pool *p, const UUID &unit_uuid);
			std::pair<bool, UUID> select_part(class Pool *p, const UUID &entity_uuid, const UUID &part_uuid, bool show_none=false);
			std::pair<bool, UUID> select_entity(class Pool *pool);
			std::pair<bool, UUID> select_padstack(class Pool *pool, const UUID &package_uuid);
			std::pair<bool, UUID> select_hole_padstack(class Pool *pool);
			std::pair<bool, UUID> select_via_padstack(class ViaPadstackProvider *vpp);
			std::pair<bool, UUID> select_net(class Block *block, bool power_only);
			std::pair<bool, UUID> select_bus(class Block *block);
			std::pair<bool, UUID> select_bus_member(class Block *block, const UUID &bus_uuid);
			bool edit_symbol_pin_names(class SchematicSymbol *symbol);
			unsigned int ask_net_merge(class Net *net, class Net *into);
			bool ask_delete_component(Component *comp);
			bool manage_buses(class Block *b);
			bool manage_net_classes(class Block *b);
			bool manage_power_nets(class Block *b);
			bool manage_via_templates(class Board *b, class ViaPadstackProvider *vpp);
			bool edit_parameter_program(class ParameterProgram *program);
			bool edit_parameter_set(ParameterSet *pset);
			bool edit_pad_parameter_set(std::set<class Pad*> &pads, class Pool *pool, class Package *pkg);
			bool edit_board_hole(std::set<class BoardHole*> &holes, class Pool *pool, class Block *block);
			bool annotate(class Schematic *s);
			bool edit_plane(class Plane *plane, class Board *brd, class Block *block);
			bool edit_stackup(class Board *brd);
			bool edit_schematic_properties(class Schematic *s);
			std::pair<bool, int64_t> ask_datum(const std::string &label, int64_t def=0);
			std::pair<bool, Coordi> ask_datum_coord(const std::string &label, Coordi def=Coordi());
			std::tuple<bool, Coordi, std::pair<bool, bool>> ask_datum_coord2(const std::string &label, Coordi def=Coordi());
			std::pair<bool, std::string> ask_datum_string(const std::string &label, const std::string &def);
			bool edit_shape(class Shape *shape);
			bool edit_via(class Via *via, class ViaPadstackProvider &vpp);
			std::tuple<bool, std::string, int, int64_t, double> ask_dxf_filename(class Core *core);


		private:
			Gtk::Window *parent = nullptr;
	};
}
