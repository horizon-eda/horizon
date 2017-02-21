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

	/**
	 * A hole with diameter and position, that's it.
	 */
	class Hole: public UUIDProvider {
		public :
			Hole(const UUID &uu, const json &j);
			Hole(const UUID &uu);

			UUID uuid;
			Coord<int64_t> position;
			uint64_t diameter=0.5_mm;
			/**
			 * true if this hole is PTH, false if NPTH.
			 * Used by the gerber exporter.
			 */
			bool plated = false;

			virtual UUID get_uuid() const ;

			//not stored
			json serialize() const;
	};
}
