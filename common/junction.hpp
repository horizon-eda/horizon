#pragma once
#include "uuid.hpp"
#include "common.hpp"
#include "object.hpp"
#include "uuid_provider.hpp"
#include "net.hpp"
#include "uuid_ptr.hpp"
#include "json_fwd.hpp"
#include "bus.hpp"
#include <vector>
#include <map>
#include <fstream>
		

namespace horizon {
	using json = nlohmann::json;
	
	
	class Junction: public UUIDProvider {
		public :
			Junction(const UUID &uu, const json &j);
			Junction(const UUID &uu);

			UUID uuid;
			Coord<int64_t> position;
			uuid_ptr<Net> net=nullptr;
			uuid_ptr<Bus> bus = nullptr;
			UUID net_segment = UUID();

			virtual UUID get_uuid() const ;
			
			//not stored
			bool temp;
			bool warning = false;
			int layer = 10000;
			bool needs_via = false;
			bool has_via = false;
			unsigned int connection_count = 0;

			json serialize() const;
	};
}
