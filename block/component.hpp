#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "common.hpp"
#include "unit.hpp"
#include "junction.hpp"
#include "line.hpp"
#include "arc.hpp"
#include "object.hpp"
#include "entity.hpp"
#include "uuid_path.hpp"
#include "uuid_ptr.hpp"
#include "net.hpp"
#include "pool.hpp"
#include <vector>
#include <map>
#include <fstream>

namespace horizon {
	using json = nlohmann::json;

	class Connection {
		public:
		Connection(const json &j, Object &obj);
		Connection(Net *n):net(n){}
		uuid_ptr<Net> net;

		json serialize() const;

	};


	class Component : public Object, public UUIDProvider {
		public :
			Component(const UUID &uu, const json &j, Pool &pool, Object &block);
			Component(const UUID &uu);


			virtual UUID get_uuid() const ;

			UUID uuid;
			Entity *entity;
			class Part *part = nullptr;
			std::string refdes;
			std::string value;
			std::map<UUIDPath<2>, Connection> connections;
			std::map<UUIDPath<2>, int> pin_names;

			json serialize() const;
			virtual ~Component() {}


		private :
			//void update_refs();
	};
}
