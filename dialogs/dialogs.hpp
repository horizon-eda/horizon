#pragma once
#include <memory>
#include "uuid.hpp"
#include "common.hpp"
#include "uuid_path.hpp"
#include "component.hpp"
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
			std::pair<bool, UUID> map_package(std::set<const Component*> &components);
			std::pair<bool, UUID> select_symbol(class Core *c, const UUID &unit_uuid);
			std::pair<bool, UUID> select_part(class Pool *p, const UUID &entity_uuid, const UUID &part_uuid);
			std::pair<bool, UUID> select_entity(class Pool *pool);
			std::pair<bool, UUID> select_padstack(class Pool *pool, const UUID &package_uuid);
			std::pair<bool, UUID> select_via_padstack(class ViaPadstackProvider *vpp);
			std::pair<bool, UUID> select_net(class Block *block, bool power_only);
			std::pair<bool, UUID> select_bus(class Block *block);
			std::pair<bool, UUID> select_bus_member(class Block *block, const UUID &bus_uuid);
			bool edit_component_pin_names(class Component *comp);
			unsigned int ask_net_merge(class Net *net, class Net *into);
			bool ask_delete_component(Component *comp);
			bool manage_buses(class Block *b);
			bool annotate(class Schematic *s);
			std::pair<bool, int64_t> ask_datum(const std::string &label, int64_t def=0);
			std::pair<bool, Coordi> ask_datum_coord(const std::string &label, Coordi def=Coordi());
			std::tuple<bool, Coordi, std::pair<bool, bool>> ask_datum_coord2(const std::string &label, Coordi def=Coordi());
			bool edit_shape(class Shape *shape);


		private:
			Gtk::Window *parent = nullptr;
	};
}
