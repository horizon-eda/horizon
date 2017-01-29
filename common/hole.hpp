#pragma once
#include "uuid.hpp"
#include "common.hpp"
#include "uuid_provider.hpp"
#include "json_fwd.hpp"
#include <vector>
#include <map>
#include <fstream>


namespace horizon {
	using json = nlohmann::json;


	class Hole: public UUIDProvider {
		public :
			Hole(const UUID &uu, const json &j);
			Hole(const UUID &uu);

			UUID uuid;
			Coord<int64_t> position;
			uint64_t diameter=0.5_mm;
			bool plated = false;

			virtual UUID get_uuid() const ;

			//not stored
			json serialize() const;
	};
}
