#pragma once
#include "uuid.hpp"
#include "json_fwd.hpp"
#include "net_class.hpp"
#include "clearance.hpp"
#include <map>

namespace horizon {
	using json = nlohmann::json;

	class Constraints {
		private :
			Constraints(const json &j);

		public :
			static Constraints new_from_file(const std::string &filename);
			Constraints();

			std::map<UUID, NetClass> net_classes;
			std::map<UUIDPath<2>, Clearance> clearances;
			Clearance *get_clearance(const NetClass *nc_a, const NetClass *nc_b);
			Clearance default_clearance;
			NetClass *default_net_class = nullptr;

			json serialize() const;
	};

}
