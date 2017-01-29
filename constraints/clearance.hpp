#pragma once
#include "uuid.hpp"
#include "uuid_path.hpp"
#include "json_fwd.hpp"
#include "net_class.hpp"
#include <stdint.h>

namespace horizon {
	using json = nlohmann::json;

	class Clearance {
		public :
			Clearance(const UUIDPath<2> &uu, const json &j, class Constraints &co);
			Clearance(const json &j);
			Clearance();
			NetClass *netclass_a = nullptr;
			NetClass *netclass_b = nullptr;
			uint64_t routing_clearance;

			json serialize() const;
	};

}
