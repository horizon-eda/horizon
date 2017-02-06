#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "pool.hpp"
#include "net.hpp"
#include "component.hpp"
#include "bus.hpp"
#include "constraints.hpp"
#include <vector>
#include <map>
#include <fstream>
#include <set>

namespace horizon {
	using json = nlohmann::json;

	class Block:public Object {
		public :
			Block(const UUID &uu, const json &, Pool &pool, Constraints *constr);
			Block(const UUID &uu);
			static Block new_from_file(const std::string &filename, Pool &pool,  Constraints *constr);
			virtual Net *get_net(const UUID &uu);
			UUID uuid;
			std::string name;
			std::map<UUID, Net> nets;
			std::map<UUID, Bus> buses;
			std::map<UUID, Component> components;

			Block(const Block &block);
			void operator=(const Block &block);

			void merge_nets(Net *net, Net *into);
			void vacuum_nets();
			Net *extract_pins(const std::set<UUIDPath<3>> &pins, Net *net=nullptr);
			void update_connection_count();
			Net *insert_net();

			json serialize();

		private:
			void update_refs();
			Constraints *constraints = nullptr;
	};

}
